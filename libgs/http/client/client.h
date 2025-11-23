
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
#include <libgs/http/client/request.h>
#include <libgs/http/client/reply.h>

namespace libgs::http
{

template <concepts::connection_pool SessionPool,
		  protocol::version_enum Version = protocol::version::v11>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using connection_pool_t = SessionPool;
	static constexpr auto version_v = Version;

	using connection_t = connection_pool_t::connection_t;
	using executor_t = connection_pool_t::executor_t;

	template <protocol::method_enum Method>
	using request_t = basic_client_request<Method, connection_t, version_v>;

	template <protocol::method_enum Method>
	using request_ptr = std::shared_ptr<request_t<Method>>;

	using reply_t = basic_reply<connection_t>;
	using reply_ptr = std::shared_ptr<reply_t>;

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
	template <typename Token>
	static constexpr bool request_token_v =
		core_concepts::tf_opt_token<Token,error_code> and
		not is_detached_v<std::remove_cvref_t<Token>>;

public:
	template <protocol::method_enum Method, typename Token = use_sync_t>
	[[nodiscard]] auto request(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <protocol::method_enum Method, typename Token = use_sync_t>
	[[nodiscard]] auto request(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <protocol::method_enum Method, typename Token = use_sync_t>
	[[nodiscard]] auto reply(const request_ptr<Method> &request, Token &&token = {});

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_get(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_put(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_post(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_head(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_patch(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_delete(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_options(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_trace(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_connect(url_t url, request_arg_t arg, Token &&token = {})
		noexcept requires request_token_v<Token>;

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_get(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_put(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_post(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_head(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_patch(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_delete(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_options(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_trace(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto req_connect(url_t url, Token &&token = {})
		noexcept requires request_token_v<Token>;

public:
	[[nodiscard]] static consteval protocol::version_enum version() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <protocol::version_enum Version = protocol::version::v11>
using client = basic_client<connection_pool, Version>;


} //namespace libgs::http
#include <libgs/http/client/detail/client.h>

namespace libgs
{

#ifdef LIBGS_ENABLE_OPENSSL
namespace https
{
// TODO ... ...
} //namespace https
#endif //LIBGS_ENABLE_OPENSSL

namespace http
{
// TODO ... ...
template <concepts::connection_pool SessionPool,
		  protocol::version_enum Version = protocol::version::v11>
class LIBGS_HTTP_TAPI basic_auto_client
{

};

}} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_CLIENT_H