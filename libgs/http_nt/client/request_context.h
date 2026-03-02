
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_CLIENT_REQUEST_CONTEXT_H
#define LIBGS_HTTP_NT_CLIENT_REQUEST_CONTEXT_H

#include <libgs/http_nt/protocol/utils/client/generator.h>
#include <libgs/http_nt/protocol/utils/client/request_arg.h>
#include <libgs/http_nt/protocol/utils/client/url.h>
#include <libgs/http_nt/client/reply.h>

namespace libgs::http_nt
{

template <method_enum Method,
		  concepts::connection Connection = connection,
		  version_enum Version = version::v11>
class LIBGS_HTTP_NT_TAPI basic_request_context final :
	public mutable_headers<basic_request_context<Method,Connection,Version>>,
	public mutable_cookies<value,basic_request_context<Method,Connection,Version>>,
	public mutable_chunk_attributes<basic_request_context<Method,Connection,Version>>
{
	LIBGS_DISABLE_COPY(basic_request_context)

public:
	using connection_t = Connection;
	using executor_t = connection_t::executor_t;

	using url_t = http_nt::url;
	using method_t = http_nt::method;
	using request_arg_t = request_arg;

	using reply_t = basic_reply<connection_t>;
	using reply_ptr = std::shared_ptr<reply_t>;

	using value_t = request_arg_t::value_t;
	using generator_t = client_generator;
	using headers_t = request_arg_t::headers_t;

	static constexpr auto method_v = Method;
	static constexpr auto version_v = Version;

	static constexpr auto put_or_post =
		method_v == method_t::post or method_v == method_t::put;

public:
	basic_request_context(connection_t &&connection, url_t url, request_arg_t arg = {});
	~basic_request_context() override;

	basic_request_context(basic_request_context &&other) noexcept;
	basic_request_context &operator=(basic_request_context &&other) noexcept;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(Token &&token = {}) noexcept;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {})
		noexcept requires put_or_post;

	template <typename T, typename Token>
	static constexpr bool file_task_token_v =
		method_v == method_t::put and
		core_concepts::tf_opt_token<Token,error_code,size_t> and
		concepts::file_opt_token_p <
			T, char, file_optype::single, io_permission::read
		>;

	template <typename T, typename Token = use_sync_t>
	auto upload_file(body_norms_t norms, T &&opt, Token &&token = {})
		noexcept requires file_task_token_v<T,Token>;

	template <typename T, typename Progress, typename Token = use_sync_t>
	auto upload_file(body_norms_t norms, T &&opt, Progress &&progress, Token &&token = {})
		noexcept requires file_task_token_v<T,Token> and concepts::progress_callback<Progress,Token>;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(const headers_t &headers, Token &&token = {})
		noexcept requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(Token &&token = {})
		noexcept requires put_or_post;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		core_concepts::tf_opt_token<Token,error_code,Value...> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename Token = use_sync_t>
	auto wait_reply(Token &&token = {}) noexcept
		requires task_token_v<Token,status_enum>;

	[[nodiscard]] const reply_ptr reply() const noexcept;
	[[nodiscard]] reply_ptr reply() noexcept;

	[[nodiscard]] bool responded() const noexcept;
	basic_request_context &cancel() noexcept;

public:
	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] request_arg_t arg() const noexcept;
	[[nodiscard]] operator request_arg_t() const noexcept;

	[[nodiscard]] static consteval method_enum method() noexcept;
	[[nodiscard]] static consteval version_enum version() noexcept;

public:
	[[nodiscard]] const connection_t &connection() const noexcept;
	[[nodiscard]] connection_t &connection() noexcept;

	[[nodiscard]] const generator_t &generator() const noexcept;
	[[nodiscard]] generator_t &generator() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <method_enum Method, version_enum Version = version::v11>
using request_context = basic_request_context<Method, connection, Version>;

} //namespace libgs::http_nt
#include <libgs/http_nt/client/detail/request_context.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http_nt
{

template <method_enum Method, version_enum Version = version::v11>
using ssl_request_context = basic_request_context<Method, ssl_connection, Version>;

} //namespace http_nt

namespace https_nt
{

template <http_nt::method_enum Method, http_nt::version_enum Version = http_nt::version::v11>
using request_context = http_nt::ssl_request_context<Method, Version>;

}} //namespace libgs::https_nt

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_NT_CLIENT_REQUEST_CONTEXT_H