
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

#ifndef LIBGS_HTTP_SERVER_SSL_SERVER_H
#define LIBGS_HTTP_SERVER_SSL_SERVER_H

#include <libgs/http/server/aop.h>
#include <libgs/io/tcp_server.h>

namespace libgs { namespace http
{

template <concept_char_type CharT, concept_execution Exec, typename Derived = void>
class LIBGS_HTTP_TAPI basic_ssl_server :
	public io::device_base<crtp_derived_t<Derived,basic_ssl_server<CharT,Exec,Derived>>, Exec>
{
	LIBGS_DISABLE_COPY(basic_ssl_server)

public:
	using executor_t = Exec;
	using next_layer_t = io::basic_tcp_server<executor_t>;
	using socket_t = io::ssl_stream<typename next_layer_t::socket_t>;

	using derived_t = crtp_derived_t<Derived,basic_ssl_server>;
	using base_t = io::device_base<derived_t,executor_t>;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using context = basic_service_context<socket_t,CharT>;
	using request_handler = std::function<awaitable<void>(context&)>;
	using system_error_handler = std::function<bool(error_code)>;
	using exception_handler = std::function<bool(context&, std::exception&)>;

	using start_token = typename next_layer_t::start_token;
	using parser = basic_request_parser<CharT>;
	using request = basic_server_request<socket_t,CharT>;
	using response = basic_server_response<request,CharT>;

	using aop = basic_aop<socket_t,CharT>;
	using ctrlr_aop = basic_ctrlr_aop<socket_t,CharT>;
	using aop_ptr = basic_aop_ptr<socket_t,CharT>;
	using ctrlr_aop_ptr = basic_ctrlr_aop_ptr<socket_t,CharT>;

public:
	explicit basic_ssl_server(size_t tcount = 0);

	template <concept_execution_context Context>
	explicit basic_ssl_server(Context &context, size_t tcount = 0);

	template <concept_execution Exec0>
	explicit basic_ssl_server(io::basic_tcp_server<executor_t> &&next_layer, size_t tcount = 0);

	explicit basic_ssl_server(const executor_t &exec, size_t tcount = 0);
	~basic_ssl_server() override;

};

} //namespace libgs::http

namespace https
{



}} //namespace libgs::https




#include <libgs/io/ssl_stream.h>

namespace libgs::http
{



} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_SSL_SERVER_H
