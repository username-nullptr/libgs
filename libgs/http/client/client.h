
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

#ifndef LIBGS_HTTP_CLIENT_CLIENT_H
#define LIBGS_HTTP_CLIENT_CLIENT_H

#include <libgs/http/client/request.h>

namespace libgs::http
{

template <core_concepts::char_type CharT,
          concepts::any_exec_stream Stream = asio::ip::tcp::socket,
		  core_concepts::execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using socket_t = Stream;
	using executor_t = Exec;

	using request_t = basic_client_request<socket_t,CharT>;
	using reply_t = typename request_t::reply_t;
	using url_t = typename request_t::url_t;

public:
	explicit basic_client(const core_concepts::match_execution<executor_t> auto &exec);
	explicit basic_client(core_concepts::match_execution_context<executor_t> auto &context);

	basic_client() requires core_concepts::match_default_execution<executor_t>;
	~basic_client();

	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;

public:
	request_t request(url_t url, error_code &error) noexcept;
	request_t request(url_t url);

public:
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec, core_concepts::execution StreamExec = asio::any_io_executor>
using basic_tcp_client = basic_client<char, asio::basic_stream_socket<asio::ip::tcp,StreamExec>, Exec>;

template <core_concepts::execution Exec, core_concepts::execution StreamExec = asio::any_io_executor>
using wbasic_tcp_client = basic_client<wchar_t, asio::basic_stream_socket<asio::ip::tcp,StreamExec>, Exec>;

using tcp_client = basic_tcp_client<asio::any_io_executor>;
using wtcp_client = wbasic_tcp_client<asio::any_io_executor>;

using client = tcp_client;
using wclient = wtcp_client;

} //namespace libgs::http
#include <libgs/http/client/detail/client.h>

#ifdef LIBGS_ENABLE_OPENSSL
// TODO ... ...
#endif //LIBGS_ENABLE_OPENSSL


#endif //LIBGS_HTTP_CLIENT_CLIENT_H
