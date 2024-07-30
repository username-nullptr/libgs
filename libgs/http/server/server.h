
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

#ifndef LIBGS_HTTP_SERVER_SERVER_H
#define LIBGS_HTTP_SERVER_SERVER_H

#include <libgs/http/server/aop.h>

#ifdef LIBGS_ENABLE_OPENSSL
# include <libgs/io/ssl_tcp_server.h>
#endif //LIBGS_ENABLE_OPENSSL

namespace libgs::http
{

template <concept_char_type CharT,
		  concept_execution MainExec = asio::any_io_executor,
		  concept_execution ServiceExec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server
{
	LIBGS_DISABLE_COPY(basic_server)

public:
	using executor_t = MainExec;
	using service_exec_t = ServiceExec;
	using next_layer_t = asio::basic_socket_acceptor<asio::ip::tcp,executor_t>;

	using endpoint_t = typename next_layer_t::endpoint_type;
	using socket_t = asio::basic_stream_socket<asio::ip::tcp,service_exec_t>;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using context = basic_service_context<socket_t,CharT>;
	using request_handler = std::function<awaitable<void>(context&)>;
	using system_error_handler = std::function<bool(error_code)>;
	using exception_handler = std::function<bool(context&, std::exception&)>;

	using parser = basic_request_parser<CharT>;
	using request = basic_server_request<socket_t,CharT>;
	using response = basic_server_response<socket_t,CharT>;

	using aop = basic_aop<socket_t,CharT>;
	using ctrlr_aop = basic_ctrlr_aop<socket_t,CharT>;
	using aop_ptr = basic_aop_ptr<socket_t,CharT>;
	using ctrlr_aop_ptr = basic_ctrlr_aop_ptr<socket_t,CharT>;

public:

	template <typename NextLayer, concept_execution_context Context = asio::io_context&>
	explicit basic_server(NextLayer &&next_layer, Context &&service_exec = execution::io_context())
		requires concept_constructible<next_layer_t,NextLayer&&>;
	~basic_server();

	template <typename MainExec0, typename ServiceExec0>
	basic_server(basic_server<CharT,MainExec0,ServiceExec0> &&other) noexcept
		requires concept_constructible<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
				 concept_constructible<service_exec_t,ServiceExec0>;

	template <typename MainExec0, typename ServiceExec0>
	basic_server &operator=(basic_server<CharT,MainExec0,ServiceExec0> &&other) noexcept
		requires concept_assignable<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
				 concept_assignable<service_exec_t,ServiceExec0>;

public:
	basic_server &bind(endpoint_t ep);
	basic_server &bind(endpoint_t ep, error_code &error) noexcept;

	basic_server &start(size_t max = asio::socket_base::max_listen_connections);
	basic_server &start(size_t max, error_code &error) noexcept;
	basic_server &start(error_code &error) noexcept;

public:
	template <http::method...method, typename Func, typename...AopPtr>
	basic_server &on_request(string_view_t path_rule, Func &&func, AopPtr&&...aops) requires
		detail::concept_request_handler<Func,socket_t,CharT> and detail::concept_aop_ptr_list<socket_t,CharT,AopPtr...>;

	template <http::method...method>
	basic_server &on_request(string_view_t path_rule, ctrlr_aop_ptr ctrlr);

	template <http::method...method>
	basic_server &on_request(string_view_t path_rule, ctrlr_aop *ctrlr);

	template <typename Func>
	basic_server &on_default(Func &&func) requires detail::concept_request_handler<Func,socket_t,CharT>;

	basic_server &on_system_error(system_error_handler func);
	basic_server &on_exception(exception_handler func);

	basic_server &unbound_request(string_view_t path_rule = {});
	basic_server &unbound_system_error();
	basic_server &unbound_exception();

	template <typename Rep, typename Period>
	basic_server &set_keepalive_time(const std::chrono::duration<Rep,Period> &d);

public:
	[[nodiscard]] const executor_t &get_executor() noexcept;
	[[nodiscard]] awaitable<void> co_stop() noexcept;
	basic_server &stop() noexcept;
	basic_server &cancel() noexcept;

public:
	[[nodiscard]] const next_layer_t &next_layer() const;
	[[nodiscard]] next_layer_t &next_layer();

private:
	class impl;
	impl *m_impl;
};

template <concept_execution MainExec, concept_execution ServiceExec = asio::any_io_executor>
using basic_tcp_server = basic_server<char,MainExec,ServiceExec>;

template <concept_execution MainExec, concept_execution ServiceExec = asio::any_io_executor>
using basic_wtcp_server = basic_server<wchar_t,MainExec,ServiceExec>;

using tcp_server = basic_tcp_server<asio::any_io_executor>;
using wtcp_server = basic_wtcp_server<asio::any_io_executor>;

using server = tcp_server;
using wserver = wtcp_server;

} //namespace libgs::http
#include <libgs/http/server/detail/server.h>

#ifdef LIBGS_ENABLE_OPENSSL
namespace libgs { namespace http
{

template <concept_execution Exec>
using basic_ssl_tcp_server = basic_server<io::basic_ssl_tcp_server<Exec>,char>;

template <concept_execution Exec>
using basic_wssl_tcp_server = basic_server<io::basic_ssl_tcp_server<Exec>,wchar_t>;

using ssl_tcp_server = basic_ssl_tcp_server<asio::any_io_executor>;
using wssl_tcp_server = basic_wssl_tcp_server<asio::any_io_executor>;

using ssl_server = ssl_tcp_server;
using wssl_server = wssl_tcp_server;

} //namespace libgs::http

namespace https
{

template <concept_execution Exec>
using basic_tcp_server = http::basic_ssl_tcp_server<Exec>;

template <concept_execution Exec>
using basic_wtcp_server = http::basic_wssl_tcp_server<Exec>;

using tcp_server = basic_tcp_server<asio::any_io_executor>;
using wtcp_server = basic_wtcp_server<asio::any_io_executor>;

using server = tcp_server;
using wserver = wtcp_server;

}} //namespace libgs::https

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_HTTP_SERVER_SERVER_H
