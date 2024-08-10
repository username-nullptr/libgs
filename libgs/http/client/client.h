
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

#include <libgs/http/client/request_helper.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_tcp_stream Stream = asio::ip::tcp::socket>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using socket_t = Stream;
	using executor_t = typename Stream::executor_type;

	using request = basic_client_request<CharT>;
//	using response = basic_client_response<CharT>;

public:
	template <concept_schedulable Exec = asio::io_context>
	explicit basic_client(Exec &exec = execution::io_context());
	~basic_client();

public:
	// template <http::method Method>
	// response request(const request &req);
	// response request(const request &req);

	// response get(const request &req);
	// response PUT(const request &req);
	// response POST(const request &req);
	// response HEAD(const request &req);
	// response DELETE(const request &req);
	// response OPTIONS(const request &req);
	// response CONNECT(const request &req);
	// response TRACH(const request &req);

public:
	[[nodiscard]] const executor_t &get_executor() noexcept;
};

template <concept_execution Exec = asio::any_io_executor>
using basic_tcp_client = basic_client<char, asio::basic_stream_socket<asio::ip::tcp,Exec>>;

template <concept_execution Exec = asio::any_io_executor>
using wbasic_tcp_client = basic_client<wchar_t, asio::basic_stream_socket<asio::ip::tcp,Exec>>;

using tcp_client = basic_tcp_client<asio::any_io_executor>;
using wtcp_client = wbasic_tcp_client<asio::any_io_executor>;

using client = tcp_client;
using wclient = wtcp_client;

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_CLIENT_H
