
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

#ifdef _WINDOWS
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#endif

#include "writer.h"
#include "libgs/core/algorithm/base.h"
#include "libgs/core/shared_mutex.h"
#include "libgs/core/app_utls.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <list>

namespace dt = std::chrono;

namespace fs = std::filesystem;

#ifdef _WINDOWS

namespace libgs::log
{

static LPSTR convert_error_code_to_string(DWORD errc)
{
	HLOCAL local_address = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, errc, 0, reinterpret_cast<PTSTR>(&local_address), 0, nullptr);
	return reinterpret_cast<LPSTR>(local_address);
}

} //namespace libgs::log

#endif //_WINDOWS

namespace std::filesystem
{

#ifdef _WINDOWS

static time_t create_time(const std::string &filename)
{
	using namespace libgs::log;

	auto hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
							TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if( hFile == INVALID_HANDLE_VALUE )
	{
		auto errc = GetLastError();
		std::cerr << "*** Error: Log: create_time: " << convert_error_code_to_string(errc) << " (" << errc << ")." << std::endl;
		return 0;
	}
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	 
	if( GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime) == 0 )
	{
		auto errc = GetLastError();
		std::cerr << "*** Error: Log: create_time: " << convert_error_code_to_string(errc) << " (" << errc << ")." << std::endl;
		return 0;
	}
	LONGLONG ll;
	ULARGE_INTEGER ui;

	ui.LowPart = creationTime.dwLowDateTime;
	ui.HighPart = creationTime.dwHighDateTime;

	ll = (static_cast<LONGLONG>(creationTime.dwHighDateTime) << 32) | creationTime.dwLowDateTime;
	return static_cast<time_t>((ui.QuadPart - 116444736000000000) / 10000000);
}

#else //other os

# include <sys/stat.h>

static time_t create_time(const std::string &filename)
{
	struct stat buf = {0};
	if( stat(filename.c_str(), &buf) < 0 )
	{
		std::cerr << "*** Error: Log: create_time: " << strerror(errno) << " (" << errno << ")." << std::endl;
		return 0;
	}
	return buf.st_ctime;
}

#endif //_WINDOWS

} //namespace std::filesystem

/*---------------------------------------------------------------------------------------------------------------*/

namespace libgs::log
{

static void _output
(output_type type, const log_context &context, const output_context &runtime_context, const std::string &msg);

static void _output
(output_type type, const log_wcontext &context, const output_wcontext &runtime_context, const std::wstring &msg);

template <concept_char_type CharT>
class LIBGS_DECL_HIDDEN writer_impl
{
	LIBGS_DISABLE_COPY_MOVE(writer_impl)

public:
	using duration = std::chrono::milliseconds;

public:
	writer_impl()
	{
		m_thread = std::thread([this]
		{
			while( not m_stop_flag )
			{
				std::unique_lock<std::mutex> locker(m_mutex);
				while( m_message_qeueue.empty() )
				{
					m_wait_flag = false;
					m_wtif_condition.notify_all();

					m_condition.wait(locker);
					if( m_stop_flag )
						return shutdown();
					m_wait_flag = true;
				}
				auto _node = m_message_qeueue.front();
				m_message_qeueue.pop_front();

				locker.unlock();
				_output(_node->type, *_node->context, _node->runtime_context, _node->msg);
			}
			m_mutex.lock();
			shutdown();
		});
	}

public:
	~writer_impl()
	{
		wait();
		m_stop_flag = true;
		m_condition.notify_one();
		try {
			if( m_thread.joinable() )
				m_thread.join();
		}
		catch(...) {}
	}
	
private:
	using context_ptr = std::shared_ptr<basic_log_context<CharT>>;
	using rt_context = basic_output_context<CharT>;
	using str_type = std::basic_string<CharT>;

	struct node
	{
		output_type type;
		context_ptr context;
		rt_context runtime_context;
		str_type msg;

		node(output_type type, context_ptr context, rt_context &&runtime_context, str_type &&msg) :
			type(type), context(std::move(context)), runtime_context(runtime_context), msg(msg) {}
	};

private:
	using node_ptr = std::shared_ptr<node>;
	using message_list = std::list<node_ptr>;
	message_list m_message_qeueue;

public:
	void produce(output_type type, const context_ptr &context, rt_context &&runtime_context, str_type &&msg)
	{
		auto _node = std::make_shared<node>(type, context, std::move(runtime_context), std::move(msg));

		m_mutex.lock();
		m_message_qeueue.emplace_back(_node);

		m_mutex.unlock();
		m_condition.notify_one();
	}

public:
	void wait(const duration &ms)
	{
		std::unique_lock<std::mutex> locker(m_wtif_mutex);
		m_wtif_condition.wait_for(locker, ms, [this]() -> bool {
			return not m_wait_flag;
		});
	}

	void wait()
	{
		std::unique_lock<std::mutex> locker(m_wtif_mutex);
		m_wtif_condition.wait(locker, [this]() -> bool {
			return not m_wait_flag;
		});
	}

private:
	void shutdown()
	{
		message_list list;
		m_message_qeueue.swap(list);
		m_mutex.unlock();

		for(auto &_node : list)
			_output(_node->type, *_node->context, _node->runtime_context, _node->msg);
	}

private:
	std::thread m_thread; // clang17 does not support std::jthread.
	std::atomic_bool m_stop_flag { false };

	std::condition_variable m_condition;
	std::mutex m_mutex;

	std::atomic_bool m_wait_flag {false};
	std::condition_variable m_wtif_condition;
	std::mutex m_wtif_mutex;
};

template <concept_char_type CharT>
static writer_impl<CharT> *g_impl = nullptr;

static std::atomic_uint g_counter {0};

/*------------------------------------------------------------------------------------------------------------------------------*/

writer::writer()
{
	if( ++g_counter == 1 )
	{
		static std::mutex mutex;
		mutex.lock();

		g_impl<char> = new writer_impl<char>();
		g_impl<wchar_t> = new writer_impl<wchar_t>();
		mutex.unlock();
	}
}

writer::~writer()
{
	if( --g_counter == 0 )
	{
		delete g_impl<char>;
		delete g_impl<wchar_t>;
	}
}

writer &writer::instance()
{
	static writer obj;
	return obj;
}

template <concept_char_type CharT>
static basic_log_context<CharT> g_context;

template <concept_char_type CharT>
static shared_mutex g_context_rwlock;

template <concept_char_type CharT>
void _write(output_type type, basic_output_context<CharT> &&runtime_context, std::basic_string<CharT> &&msg);

void writer::write(type type, output_context &&runtime_context, std::string &&msg)
{
	_write(type, std::move(runtime_context), std::move(msg));
}

void writer::write(type type, output_wcontext &&runtime_context, std::wstring &&msg)
{
	_write(type, std::move(runtime_context), std::move(msg));
}

[[nodiscard]] log_context writer::get_context() const
{
	shared_lock locker(g_context_rwlock<char>); LIBGS_UNUSED(locker);
	return g_context<char>;
}

[[nodiscard]] log_wcontext writer::get_wcontext() const
{
	shared_lock locker(g_context_rwlock<wchar_t>); LIBGS_UNUSED(locker);
	return g_context<wchar_t>;
}

static std::atomic_bool g_header_breaks_aline {false};

[[nodiscard]] bool writer::get_header_breaks_aline() const
{
	return g_header_breaks_aline;
}

template <concept_char_type CharT>
void _set_context(const basic_log_context<CharT> &context);

void writer::set_context(const log_context &con)
{
	_set_context(con);
}

void writer::set_context(const log_wcontext &con)
{
	_set_context(con);
}

void writer::set_header_breaks_aline(bool enable)
{
	g_header_breaks_aline = enable;
}

void writer::wait(const duration &ms)
{
	g_impl<char>->wait(ms);
	g_impl<wchar_t>->wait(ms);
}

void writer::wait()
{
	g_impl<char>->wait();
	g_impl<wchar_t>->wait();
}

template <concept_char_type CharT>
void _fatal(basic_output_context<CharT> &&runtime_context, const std::basic_string<CharT> &msg);

void writer::fatal(output_context &&runtime_context, const std::string &msg)
{
	_fatal(std::move(runtime_context), msg);
}

void writer::fatal(output_wcontext &&runtime_context, const std::wstring &msg)
{
	_fatal(std::move(runtime_context), msg);
}

/*------------------------------------------------------------------------------------------------------------------------------*/

template <concept_char_type CharT>
void _write(output_type type, basic_output_context<CharT> &&runtime_context, std::basic_string<CharT> &&msg)
{
	g_context_rwlock<CharT>.lock_shared();
	auto context = std::make_shared<basic_log_context<CharT>>(g_context<CharT>);
	g_context_rwlock<CharT>.unlock();

	if( context->async and type != output_type::fetal )
		g_impl<CharT>->produce(type, context, std::move(runtime_context), std::move(msg));
	else
		_output(type, *context, runtime_context, msg);
}

template <concept_char_type CharT>
void _set_context(const basic_log_context<CharT> &context)
{
	g_context_rwlock<CharT>.lock();
	g_context<CharT> = context;

	if( g_context<CharT>.category.empty() )
	{
		if constexpr( is_char_v<CharT> )
			g_context<CharT>.category = "default";
		else
			g_context<CharT>.category = L"default";
	}
	else
	{
		if constexpr( is_char_v<CharT> )
			str_replace(g_context<CharT>.category, "\\", "/");
		else
			str_replace(g_context<CharT>.category, L"\\", L"/");
	}
	str_replace(g_context<CharT>.directory, "\\", "/");

	if( not g_context<CharT>.directory.empty() )
		g_context<CharT>.directory = app::absolute_path(g_context<CharT>.directory);
	g_context_rwlock<CharT>.unlock();
}

template <concept_char_type CharT>
void _fatal(basic_output_context<CharT> &&runtime_context, const std::basic_string<CharT> &msg)
{
	g_context_rwlock<CharT>.lock_shared();
	auto context = g_context<CharT>;
	g_context_rwlock<CharT>.unlock();

	_output(output_type::fetal, context, runtime_context, msg);
	abort();
}

template <concept_char_type CharT>
static constexpr const CharT *type_string(output_type type);

template <concept_char_type CharT>
static void write_file(const std::basic_string<CharT> &log_text, const std::string &category, 
					   const basic_log_context<void> &context, const output_time &time, size_t log_size);

static std::mutex output_mutex;

static void _output
(output_type type, const log_context &context, const output_context &runtime_context, const std::string &msg)
{
	auto category = runtime_context.category.empty() ? 
						context.category == "default" ? 
							"" : 
							context.category :
						runtime_context.category;

	auto log_text = std::format("[{0}::{1}]-[{2:%Y-%m-%d %H:%M:%S}]-[{3}:{4}]:{5}", 
								category, type_string<char>(type), runtime_context.time,
								runtime_context.file, runtime_context.line, 
								g_header_breaks_aline ? "\n" : " ") + msg + "\n";
	if( g_header_breaks_aline )
		log_text += "\n";

	std::unique_lock<std::mutex> locker(output_mutex);

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
}

static void _output
(output_type type, const log_wcontext &context, const output_wcontext &runtime_context, const std::wstring &msg)
{
	auto category = runtime_context.category.empty() ?
						context.category == L"default" ?
							L"" : 
							context.category :
						runtime_context.category;

	auto log_text = std::format(L"[{0}::{1}]-[{2:%Y-%m-%d %H:%M:%S}]-[{3}:{4}]:{5}", 
								category, type_string<wchar_t>(type), runtime_context.time,
								runtime_context.file, runtime_context.line, 
								g_header_breaks_aline ? L"\n" : L" ") + msg + L"\n";
	if( g_header_breaks_aline )
		log_text += L"\n";

	std::unique_lock<std::mutex> locker(output_mutex);

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
}

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
		return "";
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
		return L"";
	}
}

static struct LIBGS_DECL_HIDDEN curr_log_file
{
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

static void size_check(const basic_log_context<void> &context, const std::string &dir_name, size_t log_size);

template <concept_char_type CharT>
static void write_file(const std::basic_string<CharT> &log_text, const std::string &category, 
					   const basic_log_context<void> &context, const output_time &time, size_t log_size)
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

static void list_push_back(std::list<fs::directory_entry> &list, const std::string &dir);
static void remove_if_too_big(const std::list<fs::directory_entry> &list, size_t max_size, size_t log_size);

static void size_check(const basic_log_context<void> &context, const std::string &dir_name, size_t log_size)
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
		return fs::create_time(info0.path().string()) < fs::create_time(info1.path().string());
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
		return fs::create_time(info0.path().string()) < fs::create_time(info1.path().string());
	});
}

static void remove_if_too_big(const std::list<fs::directory_entry> &list, size_t max_size, size_t log_size)
{
	size_t size = 0;
	for(auto &info : list)
		size += fs::file_size(info.path());

	if( size + log_size <= max_size )
		return ;

	for(auto &info : list)
	{
		size -= fs::file_size(info.path());
		fs::remove(info.path());

		if( size + log_size <= max_size )
			break;
	}
}

} //namespace libgs::log
