
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_SERVER_SERVER_H
#define LIBGS_HTTP_NT_SERVER_SERVER_H

#include <libgs/http_nt/server/acceptor_wrap.h>
#include <libgs/http_nt/server/aop.h>

namespace libgs::http_nt
{

template <concepts::any_exec_stream Stream = asio::ip::tcp::socket>
class LIBGS_HTTP_NT_TAPI basic_server
{
	LIBGS_DISABLE_COPY_MOVE(basic_server)

public:
	using socket_t = Stream;
	using executor_t = socket_t::executor_type;

	using connection_t = basic_connection<socket_t>;
	using acceptor_wrap_t = basic_acceptor_wrap<socket_t>;
	using acceptor_t = acceptor_wrap_t::acceptor_t;

	using request_t = basic_request<connection_t>;
	using response_t = basic_response<connection_t>;

public:
	template <typename  Exec0 = io_context_t&>
	explicit basic_server(acceptor_wrap_t &&acceptor, Exec0 &&exec = io_context())
		requires core_concepts::match_sched<Exec0,executor_t>;
	~basic_server();

	template <concepts::any_exec_stream Stream0>
	basic_server(basic_server<Stream0> &&other) noexcept requires
		core_concepts::constructible<acceptor_wrap_t,basic_acceptor_wrap<Stream0>>;

	template <concepts::any_exec_stream Stream0>
	basic_server &operator=(basic_server<Stream0> &&other) noexcept requires
		requires(acceptor_wrap_t &obj) { obj = std::move(other); };

public:


private:
	class impl;
	impl *m_impl;
};

template <core_concepts::exec Exec = asio::any_io_executor>
using basic_tcp_server = basic_server<asio::basic_stream_socket<asio::ip::tcp,Exec>>;

using tcp_server = basic_tcp_server<asio::any_io_executor>;
using server = tcp_server;

} //namespace libgs::http_nt
#include <libgs/http_nt/server/detail/server.h>


#endif //LIBGS_HTTP_NT_SERVER_SERVER_H