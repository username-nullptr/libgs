
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_H
#define LIBGS_HTTP_CLIENT_REQUEST_H

#include <libgs/http/protocol/utils/client/request_arg.h>
#include <libgs/http/utils/request_template.h>
#include <libgs/http/client/session_pool.h>

namespace libgs::http
{

template <protocol::method_enum Method,
		  concepts::session_pool SessionPool,
		  protocol::version_enum Version>
class client_request_targ;

template <protocol::method_enum Method,
		  concepts::session_pool SessionPool,
		  protocol::version_enum Version>
using basic_client_request = basic_request<protocol::model::client,
	client_request_targ<Method,SessionPool,Version>
>;

template <protocol::method_enum Method,
		  concepts::session_pool SessionPool,
		  protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_request<protocol::model::client,
	client_request_targ<Method,SessionPool,Version>>
{
	LIBGS_DISABLE_COPY(basic_request)

public:
	using session_pool_t = SessionPool;
	using session_t = session_pool_t::session_t;
	using executor_t = session_pool_t::executor_t;

	using request_arg_t = protocol::request_arg;
	using headers_t = request_arg_t::headers_t;

	static constexpr auto method_v = Method;
	static constexpr auto version_v = Version;

	using method_t = protocol::method;
	static constexpr auto put_or_post =
		method_v == method_t::post or method_v == method_t::put;

public:
	explicit basic_client_request(session_pool_t &pool, request_arg_t arg = {});
	~basic_client_request();

	basic_client_request(basic_client_request &&other) noexcept;
	basic_client_request &operator=(basic_client_request &&other) noexcept;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {}) requires put_or_post;

public:
	template <typename T>
	static constexpr bool file_opt_token = concepts::file_opt_token_p <
		T, char, file_optype::combine, io_permission::read
	>;
	template <typename T, core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto send_file(T &&opt, Token &&token = {}) requires file_opt_token<T> and put_or_post;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(const headers_t &headers, Token &&token = {}) requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(Token &&token = {}) requires put_or_post;

public:
	basic_client_request &set_arg(request_arg_t arg);
	[[nodiscard]] const request_arg_t &arg() const noexcept;
	[[nodiscard]] request_arg_t &arg() noexcept;

	[[nodiscard]] bool is_finished() const noexcept;
	basic_client_request &cancel() noexcept;
	basic_client_request &reset() noexcept;

public:
	[[nodiscard]] consteval protocol::method_enum method() const noexcept;
	[[nodiscard]] consteval protocol::version_enum version() const noexcept;

	[[nodiscard]] const session_pool_t &session_pool() const noexcept;
	[[nodiscard]] session_pool_t &session_pool() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <protocol::method_enum Method, protocol::version_enum Version = protocol::version::v11>
using client_request = basic_client_request<Method, session_pool, Version>;

} //namespace libgs::http
#include <libgs/http/client/detail/request.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_H