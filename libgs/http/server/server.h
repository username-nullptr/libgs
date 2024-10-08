
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

#include <libgs/http/server/acceptor_wrap.h>
#include <libgs/http/server/aop.h>

namespace libgs::http
{

template <core_concepts::char_type CharT,
		  concepts::stream_requires Stream = asio::ip::tcp::socket,
		  core_concepts::execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server
{
	LIBGS_DISABLE_COPY(basic_server)

public:
	using socket_t = Stream;
	using executor_t = Exec;
	using service_exec_t = typename socket_t::executor_type;

	using next_layer_t = basic_acceptor_wrap<socket_t>;
	using endpoint_t = typename next_layer_t::acceptor_t::endpoint_type;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using context_t = basic_service_context<socket_t,CharT>;
	using system_error_handler_t = std::function<bool(error_code)>;
	using exception_handler_t = std::function<bool(context_t&, std::exception&)>;

	using parser_t = basic_request_parser<CharT>;
	using request_t = basic_server_request<socket_t,CharT>;
	using response_t = basic_server_response<socket_t,CharT>;

	using aop_t = basic_aop<socket_t,CharT>;
	using ctrlr_aop_t = basic_ctrlr_aop<socket_t,CharT>;
	using aop_ptr_t = basic_aop_ptr<socket_t,CharT>;
	using ctrlr_aop_ptr_t = basic_ctrlr_aop_ptr<socket_t,CharT>;

public:

	template <core_concepts::execution_context Context = asio::io_context&>
	explicit basic_server(basic_acceptor_wrap<socket_t> &&next_layer, Context &&service_exec = execution::io_context());
	~basic_server();

	template <typename Stream0, typename Exec0>
	basic_server(basic_server<CharT,Stream0,Exec0> &&other) noexcept
		requires core_concepts::constructible<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
				 core_concepts::constructible<service_exec_t,typename Stream::executor_type>;

	template <typename Stream0, typename Exec0>
	basic_server &operator=(basic_server<CharT,Stream0,Exec0> &&other) noexcept
		requires core_concepts::assignable<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
				 core_concepts::assignable<service_exec_t,typename Stream::executor_type>;

public:
	basic_server &bind(endpoint_t ep);
	basic_server &bind(endpoint_t ep, error_code &error) noexcept;

	basic_server &start(size_t max = asio::socket_base::max_listen_connections);
	basic_server &start(size_t max, error_code &error) noexcept;
	basic_server &start(error_code &error) noexcept;

public:
	template <http::method...method, typename Func, typename...AopPtrs>
	basic_server &on_request(string_view_t path_rule, Func &&func, AopPtrs&&...aops) requires
		detail::concepts::request_handler<Func,socket_t,CharT> and detail::concepts::aop_ptr_list<socket_t,CharT,AopPtrs...>;

	template <http::method...method>
	basic_server &on_request(string_view_t path_rule, ctrlr_aop_ptr_t ctrlr);

	template <http::method...method>
	basic_server &on_request(string_view_t path_rule, ctrlr_aop_t *ctrlr);

	template <typename Func>
	basic_server &on_default(Func &&func) requires detail::concepts::request_handler<Func,socket_t,CharT>;

	basic_server &on_system_error(system_error_handler_t func);
	basic_server &on_exception(exception_handler_t func);

	basic_server &unbound_request(string_view_t path_rule = {});
	basic_server &unbound_system_error();
	basic_server &unbound_exception();

public:
	template <typename Rep, typename Period>
	basic_server &set_first_reading_time(const std::chrono::duration<Rep,Period> &d);

	template <typename Rep, typename Period>
	basic_server &set_keepalive_time(const std::chrono::duration<Rep,Period> &d = {});

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

template <core_concepts::execution Exec, core_concepts::execution ServiceExec = asio::any_io_executor>
using basic_tcp_server = basic_server<char, asio::basic_stream_socket<asio::ip::tcp,ServiceExec>, Exec>;

template <core_concepts::execution Exec, core_concepts::execution ServiceExec = asio::any_io_executor>
using wbasic_tcp_server = basic_server<wchar_t, asio::basic_stream_socket<asio::ip::tcp,ServiceExec>, Exec>;

using tcp_server = basic_tcp_server<asio::any_io_executor>;
using wtcp_server = wbasic_tcp_server<asio::any_io_executor>;

using server = tcp_server;
using wserver = wtcp_server;

} //namespace libgs::http
#include <libgs/http/server/detail/server.h>

#ifdef LIBGS_ENABLE_OPENSSL
namespace libgs { namespace http
{

template <concepts::execution Exec, concepts::execution ServiceExec = asio::any_io_executor>
using basic_ssl_tcp_server = basic_server<char, asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,ServiceExec>>, Exec>;

template <concepts::execution Exec, concepts::execution ServiceExec = asio::any_io_executor>
using wbasic_ssl_tcp_server = basic_server<wchar_t, asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,ServiceExec>>, Exec>;

using ssl_tcp_server = basic_ssl_tcp_server<asio::any_io_executor>;
using wssl_tcp_server = wbasic_ssl_tcp_server<asio::any_io_executor>;

using ssl_server = ssl_tcp_server;
using wssl_server = wssl_tcp_server;

} //namespace libgs::http

namespace https
{

template <concepts::execution Exec, concepts::execution ServiceExec = asio::any_io_executor>
using basic_tcp_server = http::basic_ssl_tcp_server<Exec,ServiceExec>;

template <concepts::execution Exec, concepts::execution ServiceExec = asio::any_io_executor>
using wbasic_tcp_server = http::wbasic_ssl_tcp_server<Exec,ServiceExec>;

using tcp_server = basic_tcp_server<asio::any_io_executor>;
using wtcp_server = wbasic_tcp_server<asio::any_io_executor>;

using server = tcp_server;
using wserver = wtcp_server;

}} //namespace libgs::https

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_HTTP_SERVER_SERVER_H
