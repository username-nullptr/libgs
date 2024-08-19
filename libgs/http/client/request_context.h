
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H
#define LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H

#include <libgs/http/client/request.h>
#include <libgs/http/client/session_pool.h>

namespace libgs::http
{

template <concept_tcp_stream Stream, concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_context
{
	LIBGS_DISABLE_COPY(basic_request_context)

public:
	using next_layer_t = Stream;
	using executor_t = typename next_layer_t::executor_type;

	using endpoint_t = typename socket_operation_helper<next_layer_t>::endpoint_t;
	using request_t = basic_client_request<CharT>;
	// using session_pool_t = basic_session_pool<next_layer_t>;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using header_t = basic_header<CharT>;
	using headers_t = basic_headers<CharT>;

	using value_t = basic_value<CharT>;
	using value_list_t = basic_value_list<CharT>;
	using cookies_t = basic_cookie_values<CharT>;

public:
	basic_request_context(request_t &&request/*, session_pool_t &pool*/);
	basic_request_context(/*session_pool_t &pool*/);
	~basic_request_context();

public:
	// TODO ...
	// get...
	// wait_response...

public:
	[[nodiscard]] endpoint_t remote_endpoint() const;
	[[nodiscard]] endpoint_t local_endpoint() const;

	[[nodiscard]] const executor_t &get_executor() noexcept;
	basic_request_context &cancel() noexcept;

public:
	[[nodiscard]] const request_t &request() const noexcept;
	[[nodiscard]] request_t &request() noexcept;

	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <concept_execution Exec>
using basic_tcp_request_context = basic_request_context<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using wbasic_tcp_request_context = basic_request_context<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_request_context = basic_tcp_request_context<asio::any_io_executor>;
using wtcp_request_context = wbasic_tcp_request_context<asio::any_io_executor>;

using request_context = tcp_request_context;
using wrequest_context = wtcp_request_context;

} //namespace libgs::http
#include <libgs/http/client/detail/request_context.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H

