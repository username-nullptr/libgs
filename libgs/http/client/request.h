
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_H
#define LIBGS_HTTP_CLIENT_REQUEST_H

#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/request_arg.h>
#include <libgs/http/protocol/utils/client/url.h>

#include <libgs/http/utils/multiple_template.h>
#include <libgs/http/utils/connection.h>

namespace libgs::http
{

template <protocol::method_enum Method,
		  concepts::connection Connection,
		  protocol::version_enum Version>
class client_request_targ;

template <protocol::method_enum Method,
		  concepts::connection Connection,
		  protocol::version_enum Version>
using basic_client_request = basic_request<protocol::model::client,
	client_request_targ<Method,Connection,Version>
>;

template <protocol::method_enum Method,
		  concepts::connection Connection,
		  protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_request <
	protocol::model::client, client_request_targ<Method,Connection,Version>
> final :
	public protocol::mutable_headers<basic_request <
		protocol::model::client, client_request_targ<Method,Connection,Version>
	>>,
	public protocol::mutable_cookies<value, basic_request <
		protocol::model::client, client_request_targ<Method,Connection,Version>
	>>,
	public protocol::mutable_chunk_attributes<basic_request <
		protocol::model::client, client_request_targ<Method,Connection,Version>
	>>
{
	LIBGS_DISABLE_COPY(basic_request)

public:
	using connection_t = Connection;
	using executor_t = connection_t::executor_t;

	using url_t = protocol::url;
	using request_arg_t = protocol::request_arg;
	using value_t = request_arg_t::value_t;

	using generator_t = protocol::client_generator;
	using headers_t = request_arg_t::headers_t;

	static constexpr auto method_v = Method;
	static constexpr auto version_v = Version;

	using method_t = protocol::method;
	static constexpr auto put_or_post =
		method_v == method_t::post or method_v == method_t::put;

public:
	basic_request(connection_t &&connection, url_t url, request_arg_t arg = {});
	~basic_request();

	basic_request(basic_request &&other) noexcept;
	basic_request &operator=(basic_request &&other) noexcept;

public:
	template <typename Token>
	static constexpr bool task_token_v =
		core_concepts::tf_opt_token<Token,error_code,size_t> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(Token &&token = {}) noexcept;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {})
		noexcept requires put_or_post;

	template <typename T>
	static constexpr bool file_opt_token_v =
		method_v == method_t::put and concepts::file_opt_token_p <
			T, char, file_optype::single, io_permission::read
		>;

	template <typename T, typename Token = use_sync_t>
	auto upload_file(protocol::body_norms_t norms, T &&opt, Token &&token = {})
		noexcept requires file_opt_token_v<T>;

	template <typename T, typename Progress, typename Token = use_sync_t>
	auto upload_file(protocol::body_norms_t norms, T &&opt, Progress &&progress, Token &&token = {})
		noexcept requires file_opt_token_v<T> and concepts::progress_callback<Progress,Token>;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(const headers_t &headers, Token &&token = {})
		noexcept requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(Token &&token = {})
		noexcept requires put_or_post;

public:
	basic_request &emplace(connection_t &&connection, url_t url);
	basic_request &emplace(request_arg_t arg);

	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] request_arg_t arg() const noexcept;
	[[nodiscard]] operator request_arg_t() const noexcept;

public:
	[[nodiscard]] static consteval protocol::method_enum method() noexcept;
	[[nodiscard]] static consteval protocol::version_enum version() noexcept;
	[[nodiscard]] bool is_finished() const noexcept;

	[[nodiscard]] const connection_t &connection() const noexcept;
	[[nodiscard]] connection_t &connection() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_request &cancel() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <protocol::method_enum Method, protocol::version_enum Version = protocol::version::v11>
using client_request = basic_client_request<Method, connection, Version>;

} //namespace libgs::http
#include <libgs/http/client/detail/request.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http
{

template <protocol::method_enum Method, protocol::version_enum Version = protocol::version::v11>
using ssl_client_request = basic_client_request<Method, ssl_connection, Version>;

} //namespace http

namespace https
{

template <http::protocol::method_enum Method, http::protocol::version_enum Version = http::protocol::version::v11>
using client_request = http::ssl_client_request<Method, Version>;

}} //namespace libgs::https

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_CLIENT_REQUEST_H
