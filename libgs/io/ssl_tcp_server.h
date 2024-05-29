
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

#ifndef LIBGS_IO_SSL_TCP_SERVER_H
#define LIBGS_IO_SSL_TCP_SERVER_H

#include <libgs/io/detail/tcp_server_base.h>
#include <libgs/io/ssl_tcp_socket.h>

namespace libgs::io
{

template <concept_execution Exec, typename Derived = void>
class LIBGS_IO_TAPI basic_ssl_tcp_server :
	public tcp_server_base<crtp_derived_t<Derived,basic_ssl_tcp_server<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_ssl_tcp_server)

public:
	using derived_t = crtp_derived_t<Derived,basic_ssl_tcp_server>;
	using base_t = tcp_server_base<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using socket_base_t = base_t::socket_base_t;
	using socket_t = ssl_stream<socket_base_t>;

	using native_t = base_t::native_t;
	using start_token = base_t::start_token;

public:
	using device_base<derived_t,Exec>::tcp_server_base;
	~basic_ssl_tcp_server() override = default;

public:
	template <concept_execution Exec0>
	basic_ssl_tcp_server(basic_ssl_tcp_server<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_ssl_tcp_server &operator=(basic_ssl_tcp_server<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_ssl_tcp_server &operator=(asio_basic_tcp_acceptor<Exec0> &&native) noexcept;

public:
	derived_t &accept(ssl::context &ssl, opt_token<callback_t<socket_t,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<socket_t> accept(ssl::context &ssl, opt_token<error_code&> tk = {});
};

using ssl_tcp_server = basic_ssl_tcp_server<asio::any_io_executor>;

} //namespace libgs::io
#include <libgs/io/detail/ssl_tcp_server.h>


#endif //LIBGS_IO_SSL_TCP_SERVER_H
