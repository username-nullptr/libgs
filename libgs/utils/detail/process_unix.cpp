
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

#ifdef __unix__

#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>

#include <libgs/coro/utils.h>
#include <utility>

#include <sys/wait.h>
#include <wordexp.h>
#include <pwd.h>

namespace libgs::utils::detail
{

namespace fs = std::filesystem;

using namespace std::chrono_literals;
using namespace operators;

using executor_t = process::executor_t;
using descriptor_t = asio::posix::stream_descriptor;

using read_channel_t = process::read_channel;
using args_t = std::vector<std::string>;
using envs_t = std::map<std::string, value>;

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
		if( m_stdout.is_open() )
			m_stdout.close();
		if( m_stderr.is_open() )
			m_stderr.close();
	}

public:
	[[nodiscard]] sys_expected<> start(bool is_pipe, std::string_view cmd,
		const args_t &args, std::string_view work_path, const envs_t &envs) noexcept
	{
		sys_expected<> expected;
		if( m_state == process_state::running )
			return expected;

		else if( cmd.empty() )
		{
			return expected.despair(std::make_error_code (
				std::errc::no_such_file_or_directory
			));
		}
		// From the perspective of the child process.
		int stdin_pipe [2] {-1,-1};
		int stdout_pipe[2] {-1,-1};
		int stderr_pipe[2] {-1,-1};

		if( pipe2(stdin_pipe, O_NONBLOCK) < 0 )
		{
			expected.despair(std::make_error_code (
				static_cast<std::errc>(errno)
			));
			goto stdin_error;
		}

		if( m_stdout.is_open() )
			m_stdout.close();
		if( pipe2(stdout_pipe, O_NONBLOCK) < 0 )
		{
			expected.despair(std::make_error_code (
				static_cast<std::errc>(errno)
			));
			goto stdout_error;
		}

		if( m_stderr.is_open() )
			m_stderr.close();
		if( pipe2(stderr_pipe, O_NONBLOCK) < 0 )
		{
			expected.despair(std::make_error_code (
				static_cast<std::errc>(errno)
			));
			goto stderr_error;
		}

		m_pid = vfork();
		if( m_pid < 0 )
		{
			expected.despair(std::make_error_code (
				static_cast<std::errc>(errno)
			));
			goto vfork_error;
		}
		else if( m_pid == 0 ) //child
		{
			fcntl(stdin_pipe[0], F_SETFL,
				fcntl(stdin_pipe[0], F_GETFL) | O_NONBLOCK
			);
			fcntl(stdout_pipe[1], F_SETFL,
				fcntl(stdout_pipe[1], F_GETFL) & ~O_NONBLOCK
			);
			fcntl(stderr_pipe[1], F_SETFL,
				fcntl(stderr_pipe[1], F_GETFL) & ~O_NONBLOCK
			);
			dup2(stdin_pipe [0], STDIN_FILENO );
			dup2(stdout_pipe[1], STDOUT_FILENO);
			dup2(stderr_pipe[1], STDERR_FILENO);

			int res = chdir(work_path.data());
			LIBGS_UNUSED(res);

			for(auto &[key, value] : envs)
				setenv(key.c_str(), value->c_str(), true);

			default_shell()
			.transform([&](const std::string &shell)
			{
				if( is_pipe )
					res = execlp(shell.c_str(), shell.c_str(), "-c", cmd.data(), nullptr);
				else
				{
					auto _args = new const char *[3 + args.size() + 1] {
						shell.c_str(), "-c", cmd.data(), nullptr
					};
					for(size_t i=0; i<args.size(); i++)
					{
						if( not args[i].empty() )
							_args[3 + i] = args[i].c_str();
					}
					res = execvp(shell.c_str(), const_cast<char**>(_args));
					delete[] _args;
				}
				return shell;
			})
			.or_else([&]
			{
				if( is_pipe )
				{
					expected.despair(std::make_error_code (
						std::errc::no_such_file_or_directory
					));
					_exit(-1);
				}
				auto _args = new const char *[1 + args.size() + 1] {
					cmd.data(), nullptr
				};
				for(size_t i=0; i<args.size(); i++)
				{
					if( not args[i].empty() )
						_args[1 + i] = args[i].c_str();
				}
				res = execvp(cmd.data(), const_cast<char**>(_args));
				delete[] _args;
			});
			perror("||| *** *** *** Error: execvp *** >>> ");
			_exit(res);
		}
		if( not expected )
			goto exec_error;

		m_stdin.assign(stdin_pipe[1]);
		close(stdin_pipe[0]);

		m_stdout.assign(stdout_pipe[0]);
		close(stdout_pipe[1]);

		m_stderr.assign(stderr_pipe[0]);
		close(stderr_pipe[1]);

		m_state = process_state::running;
		m_thread = std::thread([self = shared_from_this()]
		{
			int status = 0;
			while( waitpid(self->m_pid, &status, 0) < 0 )
			{
				int err = errno;
				libgs_utils_clog_error("LibGS.Utils",
					"process: waitpid failed: '{}' ({}), retrying...",
					strerror(err), err
				);
				sleep_for(10us);
			}
			if( WIFEXITED(status) )
			{
				self->m_exit_code = WEXITSTATUS(status);
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
				self->m_pid = -1;

				auto vector = std::move(self->m_co_join_list);
				for(auto &timer : vector)
					timer->cancel();
			});
			self->m_thread.detach();
			self->m_thread = {};
		});
		return expected;

	exec_error: {
		m_state = process_state::crashed;
		m_exit_code = 255;
		m_pid = -1;

		m_cv.notify_all();
		auto vector = std::move(m_co_join_list);
		for(auto &timer : vector)
			timer->cancel();
	}
	vfork_error:
		close(stderr_pipe[0]);
		close(stderr_pipe[1]);
		stderr_pipe[0] = stderr_pipe[1] = -1;

	stderr_error:
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		stdout_pipe[0] = stdout_pipe[1] = -1;

	stdout_error:
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		stdin_pipe[0] = stdin_pipe[1] = -1;

	stdin_error:
		return expected;
	}

	void _throw() noexcept
	{
		::kill(m_pid, SIGKILL);
		m_thread.join();
		std::__terminate();
	}

public:
	void terminate() const noexcept
	{
		if( m_state == process_state::running )
			::kill(m_pid, SIGTERM);
	}

	void kill() const noexcept
	{
		if( m_state == process_state::running )
			::kill(m_pid, SIGKILL);
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
	[[nodiscard]] sys_expected<int> join
	(std::chrono::nanoseconds timeout, std::error_code &error) noexcept
	{
		error.clear();
		auto state = m_state.load();

		if( state == process_state::idle )
		{
			error = make_error_code(std::errc::no_such_process);
			return sys_unexpected(error);
		}
		else if( state == process_state::crashed )
		{
			error = make_error_code(std::errc::io_error);
			return sys_unexpected(error);
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
			error = make_error_code(errc::timed_out);
			return sys_unexpected(error);
		}
		else if( state != process_state::exited )
		{
			error = make_error_code(std::errc::io_error);
			return sys_unexpected(error);
		}
		return m_exit_code.load();
	}

	[[nodiscard]] awaitable<sys_expected<int>> co_join(const std::chrono::nanoseconds &timeout,
		asio::cancellation_slot cancel_slot, std::error_code &error) noexcept
	{
		error.clear();
		auto state = m_state.load();

		if( state == process_state::idle )
		{
			error = make_error_code(std::errc::no_such_process);
			co_return sys_unexpected(error);
		}
		else if( state == process_state::crashed )
		{
			error = make_error_code(std::errc::io_error);
			co_return sys_unexpected(error);
		}
		else if( state == process_state::exited )
			co_return m_exit_code.load();

		auto exec = co_await asio::this_coro::executor;
		auto timer = std::make_shared<asio::steady_timer>(exec);

		if( timeout == 0ns )
		{
			using duration_t = asio::steady_timer::duration;
			timer->expires_after(duration_t (
				std::numeric_limits<duration_t::rep>::max()
			));
		}
		else
			timer->expires_after(timeout);

		m_co_join_list.emplace_back(timer);
		if( cancel_slot.is_connected() )
			co_await timer->async_wait(use_awaitable | cancel_slot | error);
		else
			co_await timer->async_wait(use_awaitable | error);

		state = m_state.load();
		if( state == process_state::running )
		{
			error = make_error_code (
				error ? errc::operation_aborted : errc::timed_out
			);
			co_return sys_unexpected(error);
		}
		else if( state != process_state::exited )
		{
			error = make_error_code(std::errc::io_error);
			co_return sys_unexpected(error);
		}
		error.clear();
		co_return m_exit_code.load();
	}

public:
	io_expected write(const const_buffer &buf, std::error_code &error) noexcept
	{
		error.clear();
		if( m_state != process_state::running )
		{
			error = make_error_code(std::errc::no_such_process);
			return io_unexpected(error);
		}
		error = m_stdin.non_blocking(false, error);
		if( error )
			return {error};

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
		std::error_code error;
		error = m_stdin.non_blocking(true, error);
		if( error )
		{
			libgs_utils_clog_warning("LibGS.Utils",
				"process::write<detach>: {}", error
			);
			return ;
		}
		asio::async_write(m_stdin, asio::buffer(*buf_ptr),
		[buf_ptr](const std::error_code &error, size_t)
		{
			LIBGS_UNUSED(buf_ptr);
			if( not error )
				return ;
			libgs_utils_clog_warning("LibGS.Utils",
				"process::write<detach>: {}", error
			);
		});
	}

	awaitable<io_expected> co_write(const const_buffer &buf, asio::cancellation_slot cancel_slot,
		const std::chrono::nanoseconds &timeout, std::error_code &error) noexcept
	{
		error.clear();
		if( m_state != process_state::running )
		{
			error = make_error_code(std::errc::no_such_process);
			co_return io_unexpected(error);
		}
		error = m_stdin.non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		awaitable<size_t> task {};
		if( cancel_slot.is_connected() )
		{
			task = asio::async_write(m_stdin,
				buf, use_awaitable | cancel_slot | error
			);
		}
		else
		{
			task = asio::async_write(m_stdin,
				buf, use_awaitable | error
			);
		}
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
	io_expected read(read_channel_t channel, const mutable_buffer &buf, std::error_code &error) noexcept
	{
		error.clear();
		if( m_state == process_state::idle )
		{
			error = make_error_code(std::errc::no_such_process);
			return io_unexpected(error);
		}
		size_t sum = 0;
		descriptor_t *stream = nullptr;

		if( channel == read_channel_t::stdout )
		{
			if( not m_stdout.is_open() )
			{
				error = make_error_code(std::errc::no_such_process);
				return io_unexpected(error);
			}
			stream = &m_stdout;
		}
		else
		{
			if( not m_stderr.is_open() )
			{
				error = make_error_code(std::errc::no_such_process);
				return io_unexpected(error);
			}
			stream = &m_stdout;
		}
		error = stream->non_blocking(true, error);
		if( error )
			return {error};

		char c = 0;
		auto res = ::read(m_stdout.native_handle(), &c, 1);
		if( res == 0 )
			return io_unexpected(make_error_code(errc::eof));
		else if( res == 1 )
			sum = 1;

		error = stream->non_blocking(false, error);
		if( error )
			return {error};

		auto char_buf = static_cast<char*>(buf.data());
		char_buf[0] = c;

		sum += stream->read_some (
			asio::buffer(char_buf + sum, buf.size() - sum),
			error
		);
		if( error )
			return {error};
		return sum;
	}

	void read_detach(read_channel_t channel, const mutable_buffer &buf) noexcept
	{
		if( m_state == process_state::idle )
			return ;

		std::error_code error;
		descriptor_t *stream = nullptr;

		if( channel == read_channel_t::stdout )
		{
			if( not m_stdout.is_open() )
			{
				libgs_utils_clog_warning (
					"LibGS.Utils", "process::read<detach>: {}",
					make_error_code(std::errc::no_such_process)
				);
				return ;
			}
			stream = &m_stdout;
		}
		else
		{
			if( not m_stderr.is_open() )
			{
				libgs_utils_clog_warning (
					"LibGS.Utils", "process::read_stderr<detach>: {}",
					make_error_code(std::errc::no_such_process)
				);
				return ;
			}
			stream = &m_stdout;
		}
		error = stream->non_blocking(true, error);
		if( error )
		{
			libgs_utils_clog_warning("LibGS.Utils",
				"process::read<detach>: {}", error
			);
			return ;
		}
		stream->async_read_some(buf, [](const std::error_code &error, size_t)
		{
			if( not error )
				return ;
			libgs_utils_clog_warning("LibGS.Utils",
				"process::read<detach>: {}", error
			);
		});
	}

	awaitable<io_expected> co_read(read_channel_t channel, const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, const std::chrono::nanoseconds &timeout,
		std::error_code &error) noexcept
	{
		error.clear();
		if( m_state == process_state::idle )
		{
			error = make_error_code(std::errc::no_such_process);
			co_return io_unexpected(error);
		}
		descriptor_t *stream = nullptr;

		if( channel == read_channel_t::stdout )
		{
			if( not m_stdout.is_open() )
			{
				error = make_error_code(std::errc::no_such_process);
				co_return io_unexpected(error);
			}
			stream = &m_stdout;
		}
		else
		{
			if( not m_stderr.is_open() )
			{
				error = make_error_code(std::errc::no_such_process);
				co_return io_unexpected(error);
			}
			stream = &m_stdout;
		}
		error = stream->non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		awaitable<size_t> task {};
		if( cancel_slot.is_connected() )
			task = stream->async_read_some(buf, use_awaitable | cancel_slot | error);
		else
			task = stream->async_read_some(buf, use_awaitable | error);

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
	[[nodiscard]] size_t check_time_out(const auto &var, std::error_code &error) const
	{
		if( var.index() == 0 )
			return std::get<0>(var);
		else if( not std::get<1>(var) )
			error = make_error_code(errc::timed_out);
		return 0;
	}

	[[nodiscard]] static optional<std::string> default_shell() noexcept
	{
		auto shell = getenv("SHELL");
		if( shell and strlen(shell) > 0 )
			return {shell};

		auto pw = getpwuid(getuid());
		if( pw and pw->pw_shell and strlen(pw->pw_shell) > 0 )
			return {pw->pw_shell};
		return {};
	}

public:
	std::thread m_thread {};
	int m_pid = -1;

	std::atomic<process_state> m_state {
		process_state::idle
	};
	std::atomic_int m_exit_code {0};

	// From the perspective of the child process.
	executor_t m_exec {};
	descriptor_t m_stdin  {m_exec};
	descriptor_t m_stdout {m_exec};
	descriptor_t m_stderr {m_exec};

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
	std::string m_cmd;
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

	auto cmd_str = strtls::trimmed(cmd.string());
	wordexp_t word;

	auto res = wordexp(cmd_str.c_str(), &word, 0);
	if( res != 0 )
	{
		for(auto &arg : args)
			cmd_str += " " + strtls::trimmed(arg.string());

		m_impl->m_cmd = std::move(cmd_str);
		m_impl->m_is_pipe = true;
		return ;
	}
	m_impl->m_cmd = word.we_wordv[0];
	m_impl->m_args.clear();

	for(size_t i=1; i<word.we_wordc; i++)
		m_impl->m_args.emplace_back(word.we_wordv[i]);

	for(auto &arg : args)
		add_arg(arg);
}

void process::add_arg(const path_t &arg) const noexcept
{
	if( arg.empty() )
		return ;

	else if( m_impl->m_is_pipe )
	{
		m_impl->m_cmd += " " + strtls::trimmed(arg.string());
		return ;
	}
	auto arg_str = strtls::trimmed(arg.string());
	wordexp_t word;

	auto res = wordexp(arg_str.c_str(), &word, 0);
	if( res != 0 )
	{
		for(auto &_arg : m_impl->m_args)
			m_impl->m_cmd += " " + std::move(_arg);

		m_impl->m_args.clear();
		m_impl->m_is_pipe = true;
		return ;
	}
	for(size_t i=1; i<word.we_wordc; i++)
		m_impl->m_args.emplace_back(word.we_wordv[i]);
}

sys_expected<> process::start() const noexcept
{
	return m_impl->m_vindicator->start (
		m_impl->m_is_pipe, m_impl->m_cmd, m_impl->m_args,
		m_impl->m_work_path.string(), m_impl->m_envs
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
(std::error_code &error, const std::chrono::nanoseconds &timeout) const noexcept
{
	return m_impl->m_vindicator->join(timeout, error);
}

awaitable<sys_expected<int>> process::co_join(std::error_code &error,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	return m_impl->m_vindicator->co_join(timeout, cancel_slot, error);
}

io_expected process::write(std::error_code &error, const const_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		return m_impl->m_vindicator->write(buf, error);
	return 0;
}

void process::write_detach(const const_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		m_impl->m_vindicator->write_detach(buf);
}

awaitable<io_expected> process::co_write(std::error_code &error, const const_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() > 0 )
	{
		co_return co_await m_impl->m_vindicator
			->co_write(buf, cancel_slot, timeout, error);
	}
	co_return 0;
}

io_expected process::read(read_channel channel,
	std::error_code &error, const mutable_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		return m_impl->m_vindicator->read(channel, buf, error);
	return 0;
}

void process::read_detach(read_channel channel, const mutable_buffer &buf) const noexcept
{
	if( buf.size() > 0 )
		m_impl->m_vindicator->read_detach(channel, buf);
}

awaitable<io_expected> process::co_read(read_channel channel,
	std::error_code &error, const mutable_buffer &buf, asio::cancellation_slot cancel_slot,
	std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() > 0 )
	{
		co_return co_await m_impl->m_vindicator
			->co_read(channel, buf, cancel_slot, timeout, error);
	}
	co_return 0;
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

uint64_t process::pid() const noexcept
{
	return m_impl->m_vindicator->pid();
}

} //namespace libgs::utils::detail

#endif //__unix__