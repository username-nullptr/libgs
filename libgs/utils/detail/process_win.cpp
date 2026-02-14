
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

// TODO ... ...
#pragma message("TODO ... ...")

#include <libgs/utils/process.h>

namespace libgs::utils::detail
{

namespace fs = std::filesystem;

using namespace std::chrono_literals;
using namespace operators;

using executor_t = process::executor_t;

[[nodiscard]] static error_code sys_error()
{
	return { static_cast<int>(GetLastError()), std::system_category() };
}

class LIBGS_DECL_HIDDEN process::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() {}
	~impl() {}

public:

};

process::process(const executor_t &exec) :
	m_impl(new impl())
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

}

void process::add_arg(const path_t &arg) const noexcept
{
	if( arg.empty() )
		return ;

}

sys_expected<> process::start() const noexcept
{
	return {};
}

void process::terminate() const noexcept
{
}

void process::kill() const noexcept
{
}

void process::detach() const noexcept
{
}

void process::cancel() const noexcept
{
}

sys_expected<int> process::join
(const std::chrono::nanoseconds &timeout) const noexcept
{
	return 0;
}

awaitable<sys_expected<int>> process::co_join
(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	co_return 0;
}

awaitable<sys_expected<int>> process::co_join(std::error_code &error,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	error.clear();

	co_return 0;
}

io_expected process::write(const const_buffer &buf) const noexcept
{
	return 0;
}

void process::write_detach(const const_buffer &buf) const noexcept
{
}

awaitable<io_expected> process::co_write(const const_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	co_return 0;
}

awaitable<io_expected> process::co_write(std::error_code &error, const const_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	error.clear();
	if( buf.size() == 0 )
		co_return 0;

	co_return 0;
}

io_expected process::read(read_channel channel, const mutable_buffer &buf) const noexcept
{
	return 0;
}

void process::read_detach(read_channel channel, const mutable_buffer &buf) const noexcept
{
}

awaitable<io_expected> process::co_read(read_channel channel, const mutable_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() > 0 )
	{
	}
	co_return 0;
}

awaitable<io_expected> process::co_read(
	read_channel channel, std::error_code &error, const mutable_buffer &buf,
	asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) const noexcept
{
	if( buf.size() == 0 )
		co_return 0;

	co_return 0;
}

void process::set_work_path(path_t path) noexcept
{
}

void process::setenv(std::string_view key, libgs::value value) noexcept
{
}

void process::unsetenv(std::string_view key) noexcept
{
}

process_state process::state() const noexcept
{
	return {};
}

int process::exit_code() const noexcept
{
	return -1;
}

pid_t process::pid() const noexcept
{
	return 0;
}

sys_expected<uint64_t> process::self_pid() noexcept
{
	// Always successful on unix/linux.
	return getpid();
}

sys_expected<> process::terminate(pid_t pid) noexcept
{
	return sys_unexpected(sys_error());
}

sys_expected<> process::kill(pid_t pid) noexcept
{
	return sys_unexpected(sys_error());
}

static fs::path g_pid_file {};

static sys_expected<uint64_t> do_set_single(const fs::path &path, std::string_view key)
{
	return 0;
}

sys_expected<uint64_t> process::set_single(const fs::path &path, std::string_view key)
{
	return 0;
}

} //namespace libgs::utils::detail

#endif //Windows