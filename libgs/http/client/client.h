
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#include <libgs/http/client/connection_pool.h>
#include <libgs/http/client/context.h>

namespace libgs::http
{

template <concepts::connection_pool ConnectionPool,
		  protocol::version_enum Version = protocol::version::v11>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using connection_pool_t = ConnectionPool;
	static constexpr auto version_v = Version;

	using connection_t = connection_pool_t::connection_t;
	using executor_t = connection_pool_t::executor_t;

	template <protocol::method_enum Method>
	using context_t = basic_request_context<Method,connection_t,version_v>;

	template <protocol::method_enum Method>
	using context_ptr = std::shared_ptr<context_t<Method>>;

	template <protocol::method_enum Method>
	using request_t = basic_client_request<Method, connection_t, version_v>;
	using reply_t = basic_reply<connection_t>;

	using request_arg_t = protocol::request_arg;
	using url_t = protocol::url;

public:
	basic_client() requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_client (
		core_concepts::match_sched<executor_t> auto &&exec
	);
	explicit basic_client(connection_pool_t &&pool);

	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;
	~basic_client();

public:
	struct req_info
	{
		url_t url;
		request_arg_t arg;

		req_info(url_t url, request_arg_t arg) :
			url(std::move(url)), arg(std::move(arg)) {}

		req_info(url_t url) :
			url(std::move(url)) {}

		req_info(core_concepts::string_p<char> auto &&url) :
			url(std::forward<decltype(url)>(url)) {}
	};

	template <protocol::method_enum Method, typename Token>
	static constexpr bool request_token_v =
		core_concepts::tf_opt_token<Token,error_code,context_ptr<Method>> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename T, typename Token>
	static constexpr bool file_opt_token_v =
		concepts::file_opt_token_p <
			T, char, file_optype::multiple, io_permission::read
		> and
		core_concepts::tf_opt_token <
			Token, error_code, context_ptr<protocol::method::put>
		>;

public:
	template <protocol::method_enum Method, typename Token = use_sync_t>
	[[nodiscard]] auto request(req_info info, Token &&token = {})
		noexcept requires request_token_v<Method,Token>;

	template <typename T, typename Token = use_sync_t>
	auto upload_file(req_info info, T &&opt, Token &&token = {}) noexcept
		requires file_opt_token_v<T,Token>;

	template <typename T, typename Progress, typename Token = use_sync_t>
	auto upload_file(req_info info, T &&opt, Progress &&progress, Token &&token = {}) noexcept
		requires file_opt_token_v<T,Token> and concepts::progress_callback<Progress,Token>;

	// TODO ... ...
	// download_file();

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_get(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::get,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_put(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::put,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_post(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::post,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_head(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::head,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_patch(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::patch,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_delete(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::delet,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_options(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::options,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_trace(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::trace,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto request_connect(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::connect,Token>;

public:
	template <protocol::method_enum Method, typename Token = use_sync_t>
	[[nodiscard]] auto make_context(req_info info, Token &&token = {})
		noexcept requires request_token_v<Method,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_get(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::get,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_put(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::put,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_post(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::post,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_head(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::head,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_patch(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::patch,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_delete(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::delet,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_options(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::options,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_trace(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::trace,Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto make_connect(req_info info, Token &&token = {})
		noexcept requires request_token_v<protocol::method::connect,Token>;

public:
	[[nodiscard]] static consteval protocol::version_enum version() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using client = basic_client<connection_pool>;

} //namespace libgs::http
#include <libgs/http/client/detail/client.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http {
using ssl_client = basic_client<ssl_connection_pool>;
} //namespace http

namespace https {
using client = http::ssl_client;
}} //namespace libgs::https
#endif //LIBGS_OPENSSL_SUPPORT

// TODO ... ...
namespace libgs::http
{

#if LIBGS_OPENSSL_SUPPORT
template <concepts::connection_pool ConnectionPool,
		  concepts::connection_pool SslConnectionPool,
		  protocol::version_enum Version = protocol::version::v11>
class LIBGS_HTTP_TAPI basic_auto_client
{
	// TODO ... ...
};

using auto_client = basic_auto_client <
	connection_pool, ssl_connection_pool
>;
#else //LIBGS_OPENSSL_SUPPORT
// TODO ... ...
#endif //LIBGS_OPENSSL_SUPPORT

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_CLIENT_H