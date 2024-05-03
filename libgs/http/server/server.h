
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

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_server)
	using base_type = io::device_base<Exec>;

public:
	using parser = basic_request_parser<CharT>;
	using request = basic_server_request<CharT>;
	using response = basic_server_response<CharT>;

	using request_ptr = basic_server_request_ptr<CharT,Exec>;
	using response_ptr = basic_server_response_ptr<CharT,Exec>;

	using aop = basic_aop<CharT,Exec>;
	using ctrlr_aop = basic_ctrlr_aop<CharT,Exec>;
	using aop_ptr = basic_aop_ptr<CharT,Exec>;
	using ctrlr_aop_ptr = basic_ctrlr_aop_ptr<CharT,Exec>;

	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using executor_type = typename base_type::executor_type;
	using tcp_server_type = io::basic_tcp_server<Exec>;
	using tcp_server_ptr = io::basic_tcp_server_ptr<Exec>;

	using start_token = typename tcp_server_type::start_token;
	using tcp_socket_ptr = io::tcp_socket_ptr;

	using asio_acceptor = asio_basic_tcp_acceptor<Exec>;
	using asio_acceptor_ptr = asio_basic_tcp_acceptor_ptr<Exec>;

	using context_type = basic_service_context<CharT,Exec>;
	using request_handler = std::function<awaitable<void>(context_type&)>;
	using error_handler = std::function<awaitable<void>(error_code)>;

public:
	explicit basic_server(size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution_context Context>
	explicit basic_server(Context &context, size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution Exec0>
	explicit basic_server(asio_acceptor &&acceptor, size_t tcount = std::thread::hardware_concurrency() << 1);

	explicit basic_server(const executor_type &exec, size_t tcount = std::thread::hardware_concurrency() << 1);
	~basic_server() override;

public:
	basic_server &bind(io::ip_endpoint ep, opt_token<error_code&> tk = {});
	basic_server &start(start_token tk = {});

public:
	struct aop_token
	{
		template <typename Func, typename...AopPtr>
		aop_token(Func &&callback, AopPtr&&...a) requires
			detail::concept_request_handler<Func,CharT,Exec> and
			detail::concept_aop_ptr_list<CharT,Exec,AopPtr...>;

		std::vector<aop_ptr> aops {};
		request_handler callback {};
	};

public:
	template <http::method...method>
	basic_server &on_request(str_view_type path_rule, aop_token tk);

	template <http::method...method>
	basic_server &on_request(str_view_type path_rule, ctrlr_aop_ptr ctrlr);

	template <http::method...method>
	basic_server &on_request(str_view_type path_rule, ctrlr_aop *ctrlr);

	template <typename Func>
	basic_server &on_default(Func &&callback)
		requires detail::concept_request_handler<Func,CharT,Exec>;

	basic_server &on_error(error_handler callback) noexcept;
	basic_server &unbound_request(str_view_type path_rule = {}) noexcept;
	basic_server &unbound_error() noexcept;

	template <typename Rep, typename Period>
	basic_server &set_keepalive_time(const std::chrono::duration<Rep,Period> &d);

public:
	awaitable<void> co_cancel() noexcept;
	void cancel() noexcept override;

public:
	[[nodiscard]] const tcp_server_type &native_object() const;
	[[nodiscard]] tcp_server_type &native_object();

protected:
	explicit basic_server(tcp_server_ptr tcp_server);

private:
	class impl;
	impl *m_impl;
};

using server = basic_server<char>;
using wserver = basic_server<wchar_t>;

// TODO ...
// template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
// class LIBGS_HTTP_TAPI basic_ssl_server : public basic_server<CharT,Exec>
// {
// 	// TODO ...
// };
// TODO ...
// using ssl_server = basic_ssl_server<char>;
// using ssl_wserver = basic_ssl_server<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/server.h>


#endif //LIBGS_HTTP_SERVER_SERVER_H
