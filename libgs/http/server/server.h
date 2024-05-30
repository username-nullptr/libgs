
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
#include <libgs/io/tcp_server.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec, typename Derived = void>
class LIBGS_HTTP_TAPI basic_server :
	public io::device_base<crtp_derived_t<Derived,basic_server<CharT,Exec,Derived>>, Exec>
{
	LIBGS_DISABLE_COPY(basic_server)

public:
	using executor_t = Exec;
	using next_layer_t = io::basic_tcp_server<executor_t>;
	using socket_t = typename next_layer_t::socket_t;

	using derived_t = crtp_derived_t<Derived,basic_server>;
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
	explicit basic_server(size_t tcount = 0);

	template <concept_execution_context Context>
	explicit basic_server(Context &context, size_t tcount = 0);

	template <concept_execution Exec0>
	explicit basic_server(io::basic_tcp_server<executor_t> &&next_layer, size_t tcount = 0);

	explicit basic_server(const executor_t &exec, size_t tcount = 0);
	~basic_server() override;

public:
	basic_server(basic_server &&other) noexcept;
	basic_server &operator=(basic_server &&other) noexcept;

public:
	derived_t &bind(io::ip_endpoint ep, io::no_time_token tk = {});
	derived_t &start(start_token tk = {});

public:
	template <http::method...method, typename Func, typename...AopPtr>
	derived_t &on_request(string_view_t path_rule, Func &&func, AopPtr&&...aops) requires
		detail::concept_request_handler<Func,socket_t,CharT> and detail::concept_aop_ptr_list<socket_t,CharT,AopPtr...>;

	template <http::method...method>
	derived_t &on_request(string_view_t path_rule, ctrlr_aop_ptr ctrlr);

	template <http::method...method>
	derived_t &on_request(string_view_t path_rule, ctrlr_aop *ctrlr);

	template <typename Func>
	derived_t &on_default(Func &&func) requires detail::concept_request_handler<Func,socket_t,CharT>;

	derived_t &on_system_error(system_error_handler func);
	derived_t &on_exception(exception_handler func);

	derived_t &unbound_request(string_view_t path_rule = {});
	derived_t &unbound_system_error();
	derived_t &unbound_exception();

	template <typename Rep, typename Period>
	derived_t &set_keepalive_time(const std::chrono::duration<Rep,Period> &d);

public:
	awaitable<void> co_cancel() noexcept;
	derived_t &cancel() noexcept;

public:
	[[nodiscard]] const next_layer_t &next_layer() const;
	[[nodiscard]] next_layer_t &next_layer();

private:
	class impl;
	impl *m_impl;
};

using server = basic_server<char,asio::any_io_executor>;
using wserver = basic_server<wchar_t,asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/server.h>


#endif //LIBGS_HTTP_SERVER_SERVER_H
