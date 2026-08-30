/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#if defined(__WINNT__) || defined(_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>
#include <libgs/coro/utils.h>

#include <asio/readable_pipe.hpp>
#include <asio/writable_pipe.hpp>

#include <utility>
#include <map>

#ifdef _MSC_VER
# pragma comment(lib, "shell32.lib")
#endif //_MSC_VER

namespace libgs::utils::detail
{

namespace fs = std::filesystem;

using namespace std::chrono_literals;
using namespace operators;

using executor_t = process::executor_t;
using path_t = process::path_t;

using read_pipe_t = asio::readable_pipe;
using write_pipe_t = asio::writable_pipe;

using read_channel_t = process::read_channel;

using args_t = std::vector<std::wstring>;
using envs_t = std::map<std::string, value>;

[[nodiscard]] static error_code sys_error()
{
	return { static_cast<int>(GetLastError()), std::system_category() };
}

[[nodiscard]] static std::wstring quote_arg(std::wstring_view arg)
{
	if( arg.empty() )
		return L"\"\"";

	bool need_quote = false;
	for(auto ch : arg)
	{
		if( ch == L' ' or ch == L'\t' or ch == L'\n' or ch == L'\v' or ch == L'"' )
		{
			need_quote = true;
			break;
		}
	}
	if( not need_quote )
		return std::wstring(arg);

	std::wstring result;
	result.reserve(arg.size() + 2);
	result += L'"';

	size_t backslashes = 0;
	for(auto ch : arg)
	{
		if( ch == L'\\' )
		{
			++backslashes;
			continue;
		}
		if( ch == L'"' )
		{
			result.append(backslashes * 2 + 1, L'\\');
			result += ch;
		}
		else
		{
			result.append(backslashes, L'\\');
			result += ch;
		}
		backslashes = 0;
	}
	result.append(backslashes * 2, L'\\');
	result += L'"';
	return result;
}

[[nodiscard]] static std::wstring make_cmdline(std::wstring_view cmd, const args_t &args)
{
	auto cmdline = quote_arg(cmd);
	for(auto &arg : args)
	{
		cmdline += L' ';
		cmdline += quote_arg(arg);
	}
	return cmdline;
}

[[nodiscard]] static bool has_shell_meta(std::wstring_view str) noexcept
{
	for(auto ch : str)
	{
		if( ch == L'|' or ch == L'&' or ch == L'<' or ch == L'>' )
			return true;
	}
	return false;
}

[[nodiscard]] static std::wstring to_wstring(std::string_view str)
{
	if( str.empty() )
		return {};

	int size = MultiByteToWideChar(CP_UTF8, 0, str.data(),
		static_cast<int>(str.size()), nullptr, 0
	);
	if( size <= 0 )
		return std::wstring(str.begin(), str.end());

	std::wstring result(static_cast<size_t>(size), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.data(),
		static_cast<int>(str.size()), result.data(), size
	);
	return result;
}

[[nodiscard]] static std::vector<wchar_t> make_environment(const envs_t &envs)
{
	std::map<std::wstring,std::wstring> merged;
	auto block = GetEnvironmentStringsW();
	if( block )
	{
		for(auto ptr = block; *ptr != L'\0'; )
		{
			std::wstring item = ptr;
			ptr += item.size() + 1;

			auto pos = item.find(L'=', item.starts_with(L'=') ? 1 : 0);
			if( pos != std::wstring::npos )
				merged.emplace(item.substr(0, pos), item.substr(pos + 1));
		}
		FreeEnvironmentStringsW(block);
	}
	for(auto &[key, val] : envs)
		merged[to_wstring(key)] = to_wstring(*val);

	std::vector<wchar_t> result;
	for(auto &[key, val] : merged)
	{
		result.insert(result.end(), key.begin(), key.end());
		result.push_back(L'=');
		result.insert(result.end(), val.begin(), val.end());
		result.push_back(L'\0');
	}
	result.push_back(L'\0');
	return result;
}

[[nodiscard]] static std::wstring make_pipe_name()
{
	static std::atomic_long counter {0};
	return std::format (
		LR"(\\.\pipe\libgs-utils-process-{}-{})",
		GetCurrentProcessId(), counter.fetch_add(1, std::memory_order_relaxed)
	);
}

static void close_handle(HANDLE handle) noexcept
{
	if( handle != nullptr and handle != INVALID_HANDLE_VALUE )
		CloseHandle(handle);
}

[[nodiscard]] static bool connect_pipe_server(HANDLE handle) noexcept
{
	if( ConnectNamedPipe(handle, nullptr) )
		return true;
	return GetLastError() == ERROR_PIPE_CONNECTED;
}

[[nodiscard]] static sys_expected<> make_stdout_pipe(read_pipe_t &parent, HANDLE &child) noexcept
{
	sys_expected<> expected;
	auto name = make_pipe_name();

	auto parent_handle = CreateNamedPipeW(name.c_str(),
		PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 8192, 8192, 0, nullptr
	);
	if( parent_handle == INVALID_HANDLE_VALUE )
		return expected.despair(sys_error());

	SECURITY_ATTRIBUTES sa {};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	child = CreateFileW(name.c_str(), GENERIC_WRITE, 0, &sa,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if( child == INVALID_HANDLE_VALUE )
	{
		auto error = sys_error();
		close_handle(parent_handle);
		child = nullptr;
		return expected.despair(error);
	}
	if( not connect_pipe_server(parent_handle) )
	{
		auto error = sys_error();
		close_handle(parent_handle);
		close_handle(child);
		child = nullptr;
		return expected.despair(error);
	}
	error_code error;
	parent.assign(parent_handle, error);
	if( error )
	{
		close_handle(parent_handle);
		close_handle(child);
		child = nullptr;
		return expected.despair(error);
	}
	return expected;
}

[[nodiscard]] static sys_expected<> make_stdin_pipe(write_pipe_t &parent, HANDLE &child) noexcept
{
	sys_expected<> expected;
	auto name = make_pipe_name();

	auto parent_handle = CreateNamedPipeW(name.c_str(),
		PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 8192, 8192, 0, nullptr
	);
	if( parent_handle == INVALID_HANDLE_VALUE )
		return expected.despair(sys_error());

	SECURITY_ATTRIBUTES sa {};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	child = CreateFileW(name.c_str(), GENERIC_READ, 0, &sa,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if( child == INVALID_HANDLE_VALUE )
	{
		auto error = sys_error();
		close_handle(parent_handle);
		child = nullptr;
		return expected.despair(error);
	}
	if( not connect_pipe_server(parent_handle) )
	{
		auto error = sys_error();
		close_handle(parent_handle);
		close_handle(child);
		child = nullptr;
		return expected.despair(error);
	}
	error_code error;
	parent.assign(parent_handle, error);
	if( error )
	{
		close_handle(parent_handle);
		close_handle(child);
		child = nullptr;
		return expected.despair(error);
	}
	return expected;
}

class LIBGS_DECL_HIDDEN vindicator final :
	public std::enable_shared_from_this<vindicator>
{
	LIBGS_DISABLE_COPY_MOVE(vindicator)

public:
	using ptr_t = std::shared_ptr<vindicator>;
	explicit vindicator(const executor_t &exec) :
		m_exec(exec), m_stdin(exec), m_stdout(exec), m_stderr(exec) {}

	~vindicator()
	{
		error_code error; LIBGS_UNUSED(error);
		error = m_stdin .close(error);
		error = m_stdout.close(error);
		error = m_stderr.close(error);

		if( m_process != nullptr )
			CloseHandle(m_process);
		if( m_thread.joinable() )
		{
			try {
				m_thread.detach();
			}
			catch(...) {}
		}
	}

public:
	[[nodiscard]] sys_expected<> start(bool is_pipe, std::wstring_view cmd,
		const args_t &args, const path_t &work_path, const envs_t &envs) noexcept
	{
		error_code error; LIBGS_UNUSED(error);
		sys_expected<> expected;

		if( m_state == process_state::running )
			return expected;
		else if( cmd.empty() )
		{
			return expected.despair(std::make_error_code (
				std::errc::no_such_file_or_directory
			));
		}
		write_pipe_t stdin_write(m_exec);
		read_pipe_t stdout_read(m_exec);
		read_pipe_t stderr_read(m_exec);

		HANDLE child_stdin = nullptr;
		HANDLE child_stdout = nullptr;
		HANDLE child_stderr = nullptr;

		auto pipe_expected = make_stdin_pipe(stdin_write, child_stdin);
		if( not pipe_expected )
			return pipe_expected;

		pipe_expected = make_stdout_pipe(stdout_read, child_stdout);
		if( not pipe_expected )
		{
			close_handle(child_stdin);
			return pipe_expected;
		}
		pipe_expected = make_stdout_pipe(stderr_read, child_stderr);
		if( not pipe_expected )
		{
			close_handle(child_stdin);
			close_handle(child_stdout);
			return pipe_expected;
		}

		std::wstring command_line;
		if( is_pipe )
		{
			wchar_t comspec[MAX_PATH] {};
			auto len = GetEnvironmentVariableW(L"COMSPEC", comspec, MAX_PATH);
			std::wstring shell = len > 0 ? std::wstring(comspec, len) : L"cmd.exe";
			command_line = make_cmdline(shell, {L"/C", std::wstring(cmd)});
		}
		else
			command_line = make_cmdline(cmd, args);

		STARTUPINFOW si {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = child_stdin;
		si.hStdOutput = child_stdout;
		si.hStdError = child_stderr;

		PROCESS_INFORMATION pi {};
		auto env_block = envs.empty() ? std::vector<wchar_t>{} : make_environment(envs);
		auto work_path_str = work_path.empty() ? std::wstring{} : work_path.wstring();

		if( not CreateProcessW (
			nullptr, command_line.data(), nullptr, nullptr, TRUE,
			CREATE_UNICODE_ENVIRONMENT, env_block.empty() ? nullptr : env_block.data(),
			work_path_str.empty() ? nullptr : work_path_str.c_str(), &si, &pi
		))
		{
			auto err = sys_error();
			close_handle(child_stdin);
			close_handle(child_stdout);
			close_handle(child_stderr);
			return expected.despair(err);
		}
		CloseHandle(pi.hThread);
		close_handle(child_stdin);
		close_handle(child_stdout);
		close_handle(child_stderr);

		error = m_stdin.close(error);
		error = m_stdout.close(error);
		error = m_stderr.close(error);

		error.clear();
		m_stdin = std::move(stdin_write);

		error.clear();
		m_stdout = std::move(stdout_read);

		error.clear();
		m_stderr = std::move(stderr_read);

		m_process = pi.hProcess;
		m_pid = pi.dwProcessId;
		m_state = process_state::running;

		if( m_thread.joinable() )
		{
			try {
				m_thread.detach();
			}
			catch(...) {}
		}
		m_thread = std::thread([self = shared_from_this()]
		{
			auto wait_res = WaitForSingleObject(self->m_process, INFINITE);
			DWORD exit_code = 255;
			if( wait_res == WAIT_OBJECT_0 and GetExitCodeProcess(self->m_process, &exit_code) )
			{
				self->m_exit_code = static_cast<int>(exit_code);
				self->m_state = process_state::exited;
			}
			else
			{
				self->m_exit_code = 255;
				self->m_state = process_state::crashed;
			}
			self->m_cv.notify_all();

			libgs::post(self->m_exec, [self]
			{
				self->m_stdin.close();
				self->m_pid = 0;

				auto vector = std::move(self->m_co_join_list);
				for(auto &timer : vector)
					timer->cancel();
			});
		});
		return expected;
	}

	void _throw() noexcept
	{
		TerminateProcess(m_process, static_cast<UINT>(-9));
		if( m_thread.joinable() )
			m_thread.join();
		std::terminate();
	}

public:
	void terminate() const noexcept
	{
		if( m_state == process_state::running )
			TerminateProcess(m_process, static_cast<UINT>(-9));
	}

	void kill() const noexcept
	{
		if( m_state == process_state::running )
			TerminateProcess(m_process, static_cast<UINT>(-9));
	}

	void cancel() noexcept
	{
		libgs::dispatch(m_exec, [self = shared_from_this()]
		{
			self->m_stdin .cancel();
			self->m_stdout.cancel();
			self->m_stderr.cancel();

			auto vector = std::move(self->m_co_join_list);
			for(auto &timer : vector)
				timer->cancel();
		});
	}

public:
	[[nodiscard]] sys_expected<int> join(std::chrono::nanoseconds timeout) noexcept
	{
		auto state = m_state.load();
		if( state == process_state::idle )
		{
			return sys_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		else if( state == process_state::crashed )
		{
			return sys_unexpected (
				make_error_code(std::errc::io_error)
			);
		}
		else if( state == process_state::exited )
			return m_exit_code.load();

		std::unique_lock locker(m_cv_mutex);
		if( timeout == 0ns )
		{
			m_cv.wait(locker, [self = shared_from_this()]{
				return self->m_state != process_state::running;
			});
		}
		else
		{
			m_cv.wait_for(locker, timeout, [self = shared_from_this()]{
				return self->m_state != process_state::running;
			});
		}
		state = m_state.load();
		if( state == process_state::running )
		{
			return sys_unexpected (
				make_error_code(errc::timed_out)
			);
		}
		else if( state != process_state::exited )
		{
			return sys_unexpected (
				make_error_code(std::errc::io_error)
			);
		}
		return m_exit_code.load();
	}

	[[nodiscard]] awaitable<sys_expected<int>> co_join
	(std::chrono::nanoseconds timeout, asio::cancellation_slot cancel_slot) noexcept
	{
		auto state = m_state.load();
		if( state == process_state::idle )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		else if( state == process_state::crashed )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::io_error)
			);
		}
		else if( state == process_state::exited )
			co_return m_exit_code.load();

		auto exec = co_await asio::this_coro::executor;
		auto timer = std::make_shared<asio::steady_timer>(exec);

		if( timeout == 0ns )
		{
			for(;;)
			{
				timer->expires_after(24h);
				m_co_join_list.emplace_back(timer);

				std::error_code error;
				co_await timer->async_wait (
					use_awaitable | cancel_slot | error
				);
				state = m_state.load();
				if( state == process_state::running )
				{
					if( error == errc::operation_aborted )
						co_return sys_unexpected(error);
					continue;
				}
				else if( state != process_state::exited )
				{
					co_return sys_unexpected (
						make_error_code(std::errc::io_error)
					);
				}
				break;
			}
		}
		else
		{
			timer->expires_after(timeout);
			m_co_join_list.emplace_back(timer);

			std::error_code error;
			co_await timer->async_wait (
				use_awaitable | cancel_slot | error
			);
			state = m_state.load();
			if( state == process_state::running )
			{
				co_return sys_unexpected (
					error ? error : errc::timed_out
				);
			}
			else if( state != process_state::exited )
			{
				co_return sys_unexpected (
					make_error_code(std::errc::io_error)
				);
			}
		}
		co_return m_exit_code.load();
	}

public:
	[[nodiscard]] io_expected write(const const_buffer &buf) noexcept
	{
		if( m_state != process_state::running )
		{
			return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		std::error_code error;
		auto sum = asio::write(m_stdin, buf, error);
		if( error )
			return {error};
		return sum;
	}

	void write_detach(const const_buffer &buf) noexcept
	{
		if( m_state != process_state::running )
			return ;

		auto buf_ptr = std::make_shared<std::string>(
			static_cast<const char*>(buf.data()), buf.size()
		);
		asio::async_write(m_stdin, asio::buffer(*buf_ptr),
		[buf_ptr](const std::error_code &err, size_t)
		{
			LIBGS_UNUSED(buf_ptr);
			if( not err )
				return ;
			libgs_utils_clog_warning("LibGS.Utils",
				"process::write<detach>: {}", err
			);
		});
	}

	[[nodiscard]] awaitable<io_expected> co_write(const const_buffer &buf,
		asio::cancellation_slot cancel_slot, const std::chrono::nanoseconds &timeout) noexcept
	{
		if( m_state != process_state::running )
		{
			co_return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		std::error_code error;
		auto task = asio::async_write(m_stdin,
			buf, use_awaitable | cancel_slot | error
		);
		size_t sum = 0;
		if( timeout == 0ns )
			sum = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			sum = check_time_out(var, error);
		}
		if( error )
			co_return io_unexpected(error);
		co_return sum;
	}

public:
	[[nodiscard]] io_expected read(read_channel_t channel, const mutable_buffer &buf) noexcept
	{
		if( m_state == process_state::idle )
		{
			return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		auto stream = read_stream(channel);
		if( stream == nullptr or not stream->is_open() )
		{
			return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		std::error_code error;
		auto sum = stream->read_some(buf, error);
		if( error )
			return {error};
		return sum;
	}

	void read_detach(read_channel_t channel, const mutable_buffer &buf) noexcept
	{
		if( m_state == process_state::idle )
			return ;

		auto stream = read_stream(channel);
		if( stream == nullptr or not stream->is_open() )
		{
			libgs_utils_clog_warning (
				"LibGS.Utils", "process::read<detach>: {}",
				make_error_code(std::errc::no_such_process)
			);
			return ;
		}
		stream->async_read_some(buf, [](const std::error_code &err, size_t)
		{
			if( not err )
				return ;
			libgs_utils_clog_warning("LibGS.Utils",
				"process::read<detach>: {}", err
			);
		});
	}

	awaitable<io_expected> co_read(read_channel_t channel, const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, const std::chrono::nanoseconds &timeout) noexcept
	{
		if( m_state == process_state::idle )
		{
			co_return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		auto stream = read_stream(channel);
		if( stream == nullptr or not stream->is_open() )
		{
			co_return io_unexpected (
				make_error_code(std::errc::no_such_process)
			);
		}
		std::error_code error;
		auto task = stream->async_read_some(buf,
			use_awaitable | cancel_slot | error
		);
		size_t sum = 0;
		if( timeout == 0ns )
			sum = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			sum = check_time_out(var, error);
		}
		if( error )
			co_return io_unexpected(error);
		co_return sum;
	}

public:
	[[nodiscard]] process_state state() const noexcept {
		return m_state.load();
	}
	[[nodiscard]] int exit_code() const noexcept {
		return m_exit_code.load();
	}

	[[nodiscard]] uint64_t pid() const noexcept
	{
		return m_state == process_state::running ?
			static_cast<uint64_t>(m_pid) : 0;
	}

private:
	[[nodiscard]] read_pipe_t *read_stream(read_channel_t channel) noexcept
	{
		return channel == read_channel_t::std_output ? &m_stdout : &m_stderr;
	}

	[[nodiscard]] size_t check_time_out(const auto &var, std::error_code &error) const
	{
		if( var.index() == 0 )
			return std::get<0>(var);
		else if( not std::get<1>(var) )
			error = make_error_code(errc::timed_out);
		return 0;
	}

public:
	std::thread m_thread {};
	HANDLE m_process = nullptr;
	DWORD m_pid = 0;

	std::atomic<process_state> m_state {
		process_state::idle
	};
	std::atomic_int m_exit_code {0};

	executor_t m_exec {};
	write_pipe_t m_stdin  {m_exec};
	read_pipe_t  m_stdout {m_exec};
	read_pipe_t  m_stderr {m_exec};

	std::condition_variable m_cv {};
	std::mutex m_cv_mutex {};

	std::vector <
		std::shared_ptr<asio::steady_timer>
	> m_co_join_list {};
};

using vindicator_ptr = vindicator::ptr_t;

class LIBGS_DECL_HIDDEN process::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(executor_t exec) :
		m_exec(std::move(exec)),
		m_vindicator(std::make_shared<vindicator>(m_exec)) {}

	~impl()
	{
		if( m_vindicator->state() == process_state::running )
			m_vindicator->_throw();
	}

public:
	std::wstring m_cmd;
	args_t m_args;

	path_t m_work_path;
	envs_t m_envs;

	executor_t m_exec {};
	vindicator_ptr m_vindicator {};
	bool m_is_pipe = false;
};

process::process(const executor_t &exec) :
	m_impl(new impl(exec))
{

}

process::~process()
{
	delete m_impl;
}

void process::set(const path_t &cmd, const std::vector<path_t> &args) const noexcept
{
	if( cmd.empty() )
		return ;

	auto cmd_str = strtls::trimmed(cmd.wstring());
	m_impl->m_cmd = std::move(cmd_str);
	m_impl->m_args.clear();
	m_impl->m_is_pipe = has_shell_meta(m_impl->m_cmd);

	if( not m_impl->m_is_pipe )
	{
		int count = 0;
		auto argv = CommandLineToArgvW(m_impl->m_cmd.c_str(), &count);
		if( argv != nullptr and count > 0 )
		{
			m_impl->m_cmd = argv[0];
			for(int i=1; i<count; ++i)
				m_impl->m_args.emplace_back(argv[i]);
			LocalFree(argv);
		}
		else
		{
			if( argv != nullptr )
				LocalFree(argv);
			m_impl->m_is_pipe = true;
		}
	}

	for(auto &arg : args)
		add_arg(arg);
}

void process::add_arg(const path_t &arg) const noexcept
{
	if( arg.empty() )
		return ;

	auto arg_str = strtls::trimmed(arg.wstring());
	if( m_impl->m_is_pipe )
		m_impl->m_cmd += L" " + std::move(arg_str);
	else
		m_impl->m_args.emplace_back(std::move(arg_str));
}

sys_expected<> process::start() const noexcept
{
	return m_impl->m_vindicator->start (
		m_impl->m_is_pipe, m_impl->m_cmd, m_impl->m_args,
		m_impl->m_work_path, m_impl->m_envs
	);
}

void process::terminate() const noexcept
{
	m_impl->m_vindicator->terminate();
}

void process::kill() const noexcept
{
	m_impl->m_vindicator->kill();
}

void process::detach() const noexcept
{
	if( state() == process_state::running )
		m_impl->m_vindicator = std::make_shared<vindicator>(m_impl->m_exec);
}

void process::cancel() const noexcept
{
	m_impl->m_vindicator->cancel();
}

sys_expected<int> process::join
(const std::chrono::nanoseconds &timeout) const noexcept
{
	return m_impl->m_vindicator->join(timeout);
}

awaitable<sys_expected<int>> process::co_join
(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	return m_impl->m_vindicator->co_join(timeout, cancel_slot);
}

awaitable<sys_expected<int>> process::co_join(std::error_code &error,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	error.clear();
	auto expected = co_await m_impl->m_vindicator
		->co_join(timeout, cancel_slot);

	if( not expected )
		error = expected.error();
	co_return expected;
}

io_expected process::write(const const_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		return m_impl->m_vindicator->write(buf);
	return 0;
}

void process::write_detach(const const_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		m_impl->m_vindicator->write_detach(buf);
}

awaitable<io_expected> process::co_write(const const_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() > 0 )
	{
		co_return co_await m_impl->m_vindicator
			->co_write(buf, cancel_slot, timeout);
	}
	co_return 0;
}

awaitable<io_expected> process::co_write(std::error_code &error, const const_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	error.clear();
	if( buf.size() == 0 )
		co_return 0;

	auto expected = co_await m_impl->m_vindicator
		->co_write(buf, cancel_slot, timeout);

	if( not expected )
		error = expected.error();
	co_return expected;
}

io_expected process::read(read_channel channel, const mutable_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		return m_impl->m_vindicator->read(channel, buf);
	return 0;
}

void process::read_detach(read_channel channel, const mutable_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		m_impl->m_vindicator->read_detach(channel, buf);
}

awaitable<io_expected> process::co_read(read_channel channel, const mutable_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() > 0 )
	{
		co_return co_await m_impl->m_vindicator
			->co_read(channel, buf, cancel_slot, timeout);
	}
	co_return 0;
}

awaitable<io_expected> process::co_read(
	read_channel channel, std::error_code &error, const mutable_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() == 0 )
		co_return 0;

	auto expected = co_await m_impl->m_vindicator
		->co_read(channel, buf, cancel_slot, timeout);

	if( not expected )
		error = expected.error();
	co_return expected;
}

void process::set_work_path(path_t path) noexcept
{
	m_impl->m_work_path = std::move(path);
}

void process::setenv(std::string_view key, libgs::value value) noexcept
{
	m_impl->m_envs[std::string(key)] = std::move(*value);
}

void process::unsetenv(std::string_view key) noexcept
{
	m_impl->m_envs.erase(std::string(key));
}

process_state process::state() const noexcept
{
	return m_impl->m_vindicator->state();
}

int process::exit_code() const noexcept
{
	return m_impl->m_vindicator->exit_code();
}

pid_t process::pid() const noexcept
{
	return m_impl->m_vindicator->pid();
}

sys_expected<uint64_t> process::self_pid() noexcept
{
	return GetCurrentProcessId();
}

sys_expected<> process::terminate(pid_t pid) noexcept
{
	auto handle = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
	if( handle == nullptr )
		return sys_unexpected(sys_error());

	auto ok = TerminateProcess(handle, static_cast<UINT>(-9));
	auto error = sys_error();

	CloseHandle(handle);
	if( ok )
		return {};
	return sys_unexpected(error);
}

sys_expected<> process::kill(pid_t pid) noexcept
{
	return terminate(pid);
}

static fs::path g_pid_file {};

static void singleton_cleanup()
{
	if( not g_pid_file.empty() )
		DeleteFileW(g_pid_file.wstring().c_str());
}

static sys_expected<uint64_t> do_set_single(const fs::path &path, std::string_view key)
{
	sys_expected<uint64_t> result;
	fs::path dir = path;

	if( dir.empty() )
	{
		wchar_t tmp[MAX_PATH] {};
		auto len = GetTempPathW(MAX_PATH, tmp);
		if( len == 0 or len > MAX_PATH )
			return result.despair(sys_error());
		dir = fs::path(std::wstring(tmp, len));
	}
	dir /= L".libgs.utils.process";

	std::error_code error;
	if( not fs::exists(dir, error) )
	{
		if( not fs::create_directories(dir, error) )
			return result.despair(error);
	}
	g_pid_file = dir / fs::path(std::string(key));

	auto curr_pid = GetCurrentProcessId();
	auto file = CreateFileW(g_pid_file.wstring().c_str(), GENERIC_WRITE, 0,
		nullptr, CREATE_NEW, FILE_ATTRIBUTE_READONLY, nullptr
	);
	if( file != INVALID_HANDLE_VALUE )
	{
		auto pid_str = std::to_string(curr_pid);
		DWORD written = 0; LIBGS_UNUSED(written);
		WriteFile(file, pid_str.c_str(), static_cast<DWORD>(pid_str.size()), &written, nullptr);
		CloseHandle(file);

		atexit(singleton_cleanup);
		result = curr_pid;
		return result;
	}
	auto create_error = GetLastError();
	if( create_error != ERROR_FILE_EXISTS and create_error != ERROR_ALREADY_EXISTS )
		return result.despair({ static_cast<int>(create_error), std::system_category() });

	file = CreateFileW(g_pid_file.wstring().c_str(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
	);
	if( file == INVALID_HANDLE_VALUE )
		return result.despair(sys_error());

	char buf[1024] {};
	DWORD read_len = 0;
	if( not ReadFile(file, buf, sizeof(buf) - 1, &read_len, nullptr) )
	{
		auto err = sys_error();
		CloseHandle(file);
		return result.despair(err);
	}
	CloseHandle(file);

	auto opt = strtls::to_uint64(strtls::trimmed(std::string(buf, read_len)));
	if( not opt )
	{
		SetFileAttributesW(g_pid_file.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
		DeleteFileW(g_pid_file.wstring().c_str());
		return do_set_single(path, key);
	}
	auto existing_pid = static_cast<DWORD>(*opt);
	auto process = OpenProcess(SYNCHRONIZE, FALSE, existing_pid);
	if( process != nullptr )
	{
		auto wait_res = WaitForSingleObject(process, 0);
		CloseHandle(process);
		if( wait_res == WAIT_TIMEOUT )
		{
			result = existing_pid;
			return result;
		}
	}
	SetFileAttributesW(g_pid_file.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
	if( DeleteFileW(g_pid_file.wstring().c_str()) )
		return do_set_single(path, key);
	return result.despair(sys_error());
}

sys_expected<uint64_t> process::set_single(const fs::path &path, std::string_view key)
{
	static std::atomic_bool flag {false};
	if( bool expected = false;
		not flag.compare_exchange_strong(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed) )
	{
		runtime_error::loc_throw (
			"libgs::app::set_single: Another instance is running (Prohibition of concurrent operation)."
		);
	}
	return do_set_single(path, key);
}

} //namespace libgs::utils::detail

#endif //Windows
