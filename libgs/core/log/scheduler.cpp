
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#include "scheduler.h"

#if defined(__WINNT__) || defined(_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/stat.h>
#endif

#include <libgs/core/log/mq.h>
#include <libgs/core/algorithm/base.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/shared_mutex.h>
#include <libgs/core/app_utls.h>

#include <filesystem>
#include <semaphore>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <list>
#include <map>

namespace dt = std::chrono;

namespace fs = std::filesystem;

namespace libgs::log { namespace scheduler
{

#if defined(__WINNT__) || defined(_WINDOWS)

static LPSTR convert_error_code_to_string(DWORD errc)
{
	HLOCAL local_address = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, errc, 0, reinterpret_cast<PTSTR>(&local_address), 0, nullptr);
	return reinterpret_cast<LPSTR>(local_address);
}

static time_t fs_create_time(const std::string &filename)
{
	auto hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
							TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if( hFile == INVALID_HANDLE_VALUE )
	{
		auto errc = GetLastError();
		std::cerr << "*** Error: Log: fs_create_time: " << convert_error_code_to_string(errc) << " (" << errc << ")." << std::endl;
		return 0;
	}
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;

	if( GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime) == 0 )
	{
		auto errc = GetLastError();
		std::cerr << "*** Error: Log: fs_create_time: " << convert_error_code_to_string(errc) << " (" << errc << ")." << std::endl;
		return 0;
	}
	ULARGE_INTEGER tmp;
	tmp.LowPart = creationTime.dwLowDateTime;
	tmp.HighPart = creationTime.dwHighDateTime;
	return static_cast<time_t>(tmp.QuadPart / 10000000ULL - 11644473600ULL);
}

#else //other os

static time_t fs_create_time(const std::string &filename)
{
	struct stat buf = {0};
	if( stat(filename.c_str(), &buf) < 0 )
	{
		std::cerr << "*** Error: Log: fs_create_time: " << strerror(errno) << " (" << errno << ")." << std::endl;
		return 0;
	}
	return buf.st_ctime;
}

#endif //_WINDOWS

/*---------------------------------------------------------------------------------------------------------------*/

template <concept_char_type CharT>
static constexpr const CharT *type_string(output_type type)
{
	if constexpr( is_char_v<CharT> )
	{
		switch( type )
		{
		case output_type::debug  : return "Debug";
		case output_type::info   : return "Info";
		case output_type::warning: return "Warning";
		case output_type::error  : return "Error";
		case output_type::fetal  : return "Fetal";
		default: break;
		}
	}
	else
	{
		switch( type )
		{
		case output_type::debug  : return L"Debug";
		case output_type::info   : return L"Info";
		case output_type::warning: return L"Warning";
		case output_type::error  : return L"Error";
		case output_type::fetal  : return L"Fetal";
		default: break;
		}
	}
	throw runtime_error("Code bug: enum output_type is invalid.");
}

static struct LIBGS_DECL_HIDDEN curr_log_file
{
	LIBGS_DISABLE_COPY_MOVE(curr_log_file)
	curr_log_file() = default;

	std::map<std::string, std::string> name_map;
	output_time last;

	void save_last_name(const std::string &category)
	{
		auto &name = name_map[category];
		if( name.empty() )
			return ;

		auto old_name = name;
		name.erase(name.size() - 5);

		name += std::format("{:%H:%M:%S}", last);
		std::rename(old_name.c_str(), name.c_str());
	}
}
g_curr_log_file;

static void list_push_back(std::list<fs::directory_entry> &list, const std::string &dir)
{
	try {
		for(auto &entry : fs::directory_iterator(dir))
		{
			if( not entry.is_directory() )
				list.emplace_back(entry);
		}
	}
	catch(...)
	{
		std::cerr << "*** Error: Log: open_log_output_device: Unable to traverse directory '" << dir << "'." << std::endl;
		return ;
	}
	list.sort([](const fs::directory_entry &info0, const fs::directory_entry &info1) {
		return fs_create_time(info0.path().string()) < fs_create_time(info1.path().string());
	});
}

static void remove_if_too_big(const std::list<fs::directory_entry> &list, size_t max_size, size_t log_size)
{
	size_t size = 0;
	for(auto &info : list)
		size += static_cast<size_t>(fs::file_size(info.path()));

	if( size + log_size <= max_size )
		return ;

	for(auto &info : list)
	{
		size -= static_cast<size_t>(fs::file_size(info.path()));
		fs::remove(info.path());

		if( size + log_size <= max_size )
			break;
	}
}

static void size_check(const basic_static_context<void> &context, const std::string &dir_name, size_t log_size)
{
	std::list<fs::directory_entry> file_info_list;

	list_push_back(file_info_list, dir_name);
	remove_if_too_big(file_info_list, context.max_size_one_day, log_size);

	std::list<fs::directory_entry> dir_info_list;
	try {
		for(auto &entry : fs::directory_iterator(dir_name))
		{
			if( entry.is_directory() )
				dir_info_list.emplace_back(entry);
		}
	}
	catch(...)
	{
		std::cerr << "*** Error: Log: open_log_output_device: Unable to traverse directory '" << dir_name << "'." << std::endl;
		return ;
	}
	dir_info_list.sort([](const fs::directory_entry &info0, const fs::directory_entry &info1) {
		return fs_create_time(info0.path().string()) < fs_create_time(info1.path().string());
	});

	file_info_list.clear();
	for(auto &info : dir_info_list)
		list_push_back(file_info_list, info.path().string());
	remove_if_too_big(file_info_list, context.max_size, log_size);

	if( dir_info_list.size() == 1 )
		return ;
	for(auto &info : dir_info_list)
	{
		if( not fs::is_directory(info.path()) )
			continue;

		fs::directory_iterator dir_iter;
		try {
			dir_iter = fs::directory_iterator(info.path());
		}
		catch(...) {
			continue;
		}
		if( dir_iter == fs::end(dir_iter) )
			fs::remove_all(info.path());
		else
			break;
	}
}

template <concept_char_type CharT>
static void write_file(const std::basic_string<CharT> &log_text, const std::string &category,
					   const basic_static_context<void> &context, const output_time &time, size_t log_size)
{
	if( context.directory.empty() )
		return ;

	auto &curr_file_name = g_curr_log_file.name_map[category];
	if( dt::time_point_cast<dt::days>(time) > g_curr_log_file.last )
	{
		g_curr_log_file.save_last_name(category);
		curr_file_name = "";
	}
	auto dir_name = context.directory;
	if( not dir_name.ends_with("/") )
		dir_name += "/";

	if( context.time_category )
	{
		dir_name += std::format("{:%Y-%m-%d}", time) + "/" + category;
		if( not dir_name.ends_with("/") )
			dir_name += "/";
	}
	else
	{
		dir_name += category;
		if( not dir_name.ends_with("/") )
			dir_name += "/";
		dir_name += std::format("{:%Y-%m-%d}", time) + "/";
	}
	std::error_code error;
	if( not fs::exists(dir_name) )
	{
		fs::create_directories(dir_name, error);
		if( error )
		{
			std::cerr << "*** Error: Log: open_log_output_device: Log directory make failed: " << error.message() << std::endl;
			return;
		}
	}
	else if( not fs::is_directory(dir_name) )
	{
		std::cerr << "*** Error: Log: open_log_output_device: '" << dir_name << "' exists, but it is not a directory." << std::endl;
		return ;
	}
	if( curr_file_name.empty() )
	{
		try {
			for(auto &entry : fs::directory_iterator(dir_name))
			{
				auto file_name = entry.path().string();
				if( not file_name.ends_with("(now)") )
					continue;

				curr_file_name = file_name;
				break;
			}
		}
		catch(...)
		{
			std::cerr << "*** Error: Log: open_log_output_device: Unable to traverse directory '" << dir_name << "'." << std::endl;
			return ;
		}
		if( curr_file_name.empty() )
			curr_file_name = dir_name + std::format("{:%H:%M:%S}-(now)", time);
	}
	else if( fs::file_size(curr_file_name) + log_size > context.max_size_one_file )
	{
		g_curr_log_file.save_last_name(category);
		curr_file_name = dir_name + std::format("{:%H:%M:%S}-(now)", time);
	}
	size_check(context, dir_name, log_size);
	g_curr_log_file.last = time;

	std::basic_ofstream<CharT> file(curr_file_name, std::ios_base::out | std::ios_base::app);
	if( not file.is_open() )
	{
		fprintf(stderr, "*** Error: Log: openLogOutputDevice: The log file '%s' open failed: %s.\n",
				curr_file_name.c_str(),
#ifdef _WINDOWS
				convert_error_code_to_string(GetLastError())
#else
				strerror(errno)
#endif //_WINDOWS
		);
	}
	file << log_text;
	file.close();
}

static std::counting_semaphore<1> g_output_semaphore {1};

static void _output
(output_type type, const static_context &context, const output_context &runtime_context, const std::string &msg)
{
	auto category = runtime_context.category.empty() ?
						context.category == "default" ?
							"" :
							context.category :
						runtime_context.category;
	std::string source;
	if( context.source_visible )
		source = std::format("-[{0}:{1}]", context.file_path_visible ? runtime_context.file : file_name(runtime_context.file), runtime_context.line);

	auto log_text = std::format("[{0}::{1}]-[{2:%Y-%m-%d %H:%M:%S}]{3}:{4}",
								category, type_string<char>(type), runtime_context.time,
								source, context.header_breaks_aline ? "\n" : " ") + msg + "\n";
	if( context.header_breaks_aline )
		log_text += "\n";

	// No lock is required when there is only one thread.
	g_output_semaphore.acquire();

	if( type == output_type::debug )
	{
		if( context.mask < 4 )
			return ;
		if( context.no_stdout )
			std::cerr << log_text << std::flush;
		else
			std::cout << log_text << std::flush;
	}
	else if( type == output_type::info )
	{
		if( context.mask < 3 )
			return ;
		if( context.no_stdout )
			std::cerr << log_text << std::flush;
		else
			std::cout << log_text << std::flush;
	}
	else if( type == output_type::warning )
	{
		if( context.mask < 2 )
			return ;
		std::cerr << log_text << std::flush;
	}
	else if( type == output_type::error )
	{
		if( context.mask < 1 )
			return ;
		std::cerr << log_text << std::flush;
	}
	else
		std::cerr << log_text << std::flush;

	write_file<char>(log_text, category, context, runtime_context.time, log_text.size());
	g_output_semaphore.release(1);
}

static void _output
(output_type type, const static_context &context, const output_wcontext &runtime_context, const std::wstring &msg)
{
	auto category = runtime_context.category.empty() ?
						context.category == "default" ?
							L"" :
							mbstowcs(context.category) :
						runtime_context.category;
	std::wstring source;
	if( context.source_visible )
		source = std::format(L"-[{0}:{1}]", context.file_path_visible ? runtime_context.file : file_name(runtime_context.file), runtime_context.line);

	auto log_text = std::format(L"[{0}::{1}]-[{2:%Y-%m-%d %H:%M:%S}]{3}:{4}",
								category, type_string<wchar_t>(type), runtime_context.time,
								source, context.header_breaks_aline ? L"\n" : L" ") + msg + L"\n";
	if( context.header_breaks_aline )
		log_text += L"\n";

	// No lock is required when there is only one thread.
	g_output_semaphore.acquire();

	if( type == output_type::debug )
	{
		if( context.mask < 4 )
			return ;
		if( context.no_stdout )
			std::wcerr << log_text << std::flush;
		else
			std::wcout << log_text << std::flush;
	}
	else if( type == output_type::info )
	{
		if( context.mask < 3 )
			return ;
		if( context.no_stdout )
			std::wcerr << log_text << std::flush;
		else
			std::wcout << log_text << std::flush;
	}
	else if( type == output_type::warning )
	{
		if( context.mask < 2 )
			return ;
		std::wcerr << log_text << std::flush;
	}
	else if( type == output_type::error )
	{
		if( context.mask < 1 )
			return ;
		std::wcerr << log_text << std::flush;
	}
	else
		std::wcerr << log_text << std::flush;

	write_file<wchar_t>(log_text, libgs::wcstombs(category), context, runtime_context.time, log_text.size());
	g_output_semaphore.release(1);
}

static void output(msg_node_base_ptr &_node_base)
{
	{
		auto _node = std::dynamic_pointer_cast<message_node>(_node_base);
		if( _node )
			return _output(_node->type, *_node->context, _node->runtime_context, _node->msg);
	}{
		auto _node = std::dynamic_pointer_cast<wmessage_node>(_node_base);
		assert(_node);
		_output(_node->type, *_node->context, _node->runtime_context, _node->msg);
	}
}

static static_context g_context;

static shared_mutex g_context_rwlock;

static void context_transition(auto &context, const auto &other)
{
	context.directory           = other.directory;
	context.max_size_one_file   = other.max_size_one_file;
	context.max_size_one_day    = other.max_size_one_day ;
	context.max_size            = other.max_size;
	context.mask                = other.mask;
	context.source_visible      = other.source_visible;
	context.file_path_visible   = other.file_path_visible;
	context.time_category       = other.time_category;
	context.async               = other.async;
	context.no_stdout           = other.no_stdout;
	context.header_breaks_aline = other.header_breaks_aline;
}

static class LIBGS_DECL_HIDDEN scheduler_impl
{
	LIBGS_DISABLE_COPY_MOVE(scheduler_impl)
	using context_ptr = std::shared_ptr<static_context>;

	std::thread m_thread; // clang17 does not support std::jthread.
	std::atomic_bool m_stop_flag {false};

	std::counting_semaphore<> m_semaphore {0};
	message_queue m_message_qeueue;

public:
	scheduler_impl()
	{
		m_thread = std::thread([this]
		{
			for(;;)
			{
				m_semaphore.acquire();
				if( m_stop_flag )
					break;

				auto opd = m_message_qeueue.dequeue();
				assert(opd);
				output(*opd);
			}
			shutdown();
		});
	}

	~scheduler_impl()
	{
		if( not m_stop_flag )
			on_exit();
	}

public:
	template <concept_char_type CharT>
	void write(output_type type, basic_output_context<CharT> &&runtime_context, std::basic_string<CharT> &&msg)
	{
		g_context_rwlock.lock_shared();
		auto context = std::make_shared<static_context>(g_context);
		g_context_rwlock.unlock_shared();

		if( context->async and type != output_type::fetal )
			produce<CharT>(type, context, std::move(runtime_context), std::move(msg));
		else
			_output(type, *context, runtime_context, msg);
	}

	template <concept_char_type CharT>
	void fatal(basic_output_context<CharT> &&runtime_context, const std::basic_string<CharT> &msg)
	{
		g_context_rwlock.lock_shared();
		auto context = g_context;
		g_context_rwlock.unlock_shared();

		_output(output_type::fetal, context, runtime_context, msg);
		abort();
	}

public:
	void on_exit()
	{
		m_stop_flag = true;
		m_semaphore.release(1);
		try {
			if( m_thread.joinable() )
				m_thread.join();
		}
		catch(...) {}
	}

private:
	template <concept_char_type CharT>
	void produce(output_type type, const context_ptr &context,
				 basic_output_context<CharT> &&runtime_context, std::basic_string<CharT> &&msg)
	{
		auto _node = std::make_shared<basic_message_node<CharT>>(type, context, std::move(runtime_context), std::move(msg));
		m_message_qeueue.enqueue(_node);
		m_semaphore.release(1);
	}

	void shutdown()
	{
		while( auto opd = m_message_qeueue.dequeue() )
		{
			assert(opd);
			output(*opd);
		}
	}
}
g_impl;

/*------------------------------------------------------------------------------------------------------------------------------*/

void set_context(static_context context)
{
	if( context.category.empty() )
		context.category = "default";
	else
		str_replace(context.category, "\\", "/");

	str_replace(context.directory, "\\", "/");

	if( not context.directory.empty() )
		context.directory = app::absolute_path(context.directory);

	g_context_rwlock.lock();
	g_context = std::move(context);
	g_context_rwlock.unlock();
}

void set_context(static_wcontext context)
{
	static_context tmp;
	context_transition(tmp, context);

	tmp.category = wcstombs(context.category);
	set_context(std::move(tmp));
}

static_context get_context()
{
	unique_lock locker(g_context_rwlock); LIBGS_UNUSED(locker);
	return g_context;
}

static_wcontext get_wcontext()
{
	static_wcontext tmp;
	g_context_rwlock.lock_shared();

	context_transition(tmp, g_context);
	tmp.category = mbstowcs(g_context.category);

	g_context_rwlock.unlock_shared();
	return tmp;
}

void write(output_type type, output_context &&runtime_context, std::string &&msg)
{
	g_impl.write(type, std::move(runtime_context), std::move(msg));
}

void write(output_type type, output_wcontext &&runtime_context, std::wstring &&msg)
{
	g_impl.write(type, std::move(runtime_context), std::move(msg));
}

void fatal(output_context &&runtime_context, const std::string &msg)
{
	g_impl.fatal(std::move(runtime_context), msg);
}

void fatal(output_wcontext &&runtime_context, const std::wstring &msg)
{
	g_impl.fatal(std::move(runtime_context), msg);
}

void exit()
{
	g_impl.on_exit();
}

} //namespace scheduler

void exit()
{
	scheduler::exit();
}

} //namespace libgs::log

#if defined(__WINNT__) || defined(_WINDOWS)
BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
	if( fdwReason == DLL_PROCESS_DETACH )
		libgs::log::scheduler::g_impl.on_exit();
	return TRUE;
}
#endif //__WINDOWS