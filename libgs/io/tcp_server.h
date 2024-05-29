
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

#ifndef LIBGS_IO_TCP_SERVER_H
#define LIBGS_IO_TCP_SERVER_H

#include <libgs/io/tcp_socket.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor = asio::basic_socket_acceptor<asio::ip::tcp, Exec>;

using asio_tcp_acceptor = asio_basic_tcp_acceptor<>;

namespace io
{

template <concept_execution Exec, typename Derived = void>
class LIBGS_IO_TAPI basic_tcp_server : public device_base<crtp_derived_t<Derived,basic_tcp_server<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_tcp_server)

public:
	using derived_t = crtp_derived_t<Derived,basic_tcp_server>;
	using base_t = device_base<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using socket_t = tcp_socket;
	using native_t = asio_basic_tcp_acceptor<executor_t>;

	struct start_token : no_time_token
	{
		size_t max = asio::socket_base::max_listen_connections;
		using no_time_token::no_time_token;
		start_token(size_t max, error_code &error);
		start_token(size_t max);
	};

public:
	explicit basic_tcp_server(size_t tcount = 0);

	template <concept_execution_context Context>
	explicit basic_tcp_server(Context &context, size_t tcount = 0);

	template <concept_execution Exec0>
	explicit basic_tcp_server(asio_basic_tcp_acceptor<Exec0> &&native, size_t tcount = 0);

	explicit basic_tcp_server(const executor_t &exec, size_t tcount = 0);
	~basic_tcp_server() override;

public:
	basic_tcp_server(basic_tcp_server &&other) noexcept = default;
	basic_tcp_server &operator=(basic_tcp_server &&other) noexcept = default;
	basic_tcp_server &operator=(native_t &&native) noexcept;

public:
	derived_t &bind(ip_endpoint ep, opt_token<error_code&> tk = {});
	derived_t &start(start_token tk = {});

	derived_t &accept(opt_token<callback_t<socket_t,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<socket_t> accept(opt_token<error_code&> tk = {});

public:
	derived_t &set_option(const auto &op, no_time_token tk = {});
	derived_t &get_option(auto &op, no_time_token tk = {});
	const derived_t &get_option(auto &op, no_time_token tk = {}) const;

public:
	awaitable<void> co_wait() noexcept;
	derived_t &wait() noexcept;

	awaitable<void> co_stop() noexcept;
	derived_t &stop() noexcept;
	derived_t &cancel() noexcept;

	const asio::thread_pool &pool() const noexcept;
	asio::thread_pool &pool() noexcept;

public:
	const native_t &native() const noexcept;
	native_t &native() noexcept;

protected:
	static size_t _tcount(size_t c) noexcept;
	native_t m_native;
	asio::thread_pool m_pool;
};

using tcp_server = basic_tcp_server<asio::any_io_executor>;

}} //namespace libgs::io
#include <libgs/io/detail/tcp_server.h>


#endif //LIBGS_IO_TCP_SERVER_H
