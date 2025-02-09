
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

#include <libgs/http/client/reply.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, method Method,
		  concepts::session_pool SessionPool = session_pool,
		  version_t Version = version::v11>
class LIBGS_HTTP_TAPI basic_client_request
{
	LIBGS_DISABLE_COPY(basic_client_request)

public:
	using char_t = CharT;
	using session_pool_t = SessionPool;
	using session_t = typename session_pool_t::session_t;

	using map_helper_t = basic_attr_map_helper<char_t>;
	using reply_t = basic_client_reply<char_t,session_t>;
	using request_arg_t = basic_request_arg<char_t>;

	using headers_t = typename request_arg_t::headers_t;
	using executor_t = typename session_pool_t::executor_t;

	static constexpr auto method_v = Method;
	static constexpr auto version_v = Version;

	static constexpr auto put_or_post =
		method_v == method_t::POST or method_v == method_t::PUT;

public:
	basic_client_request(session_pool_t &pool, request_arg_t arg = {});
	~basic_client_request();

	basic_client_request(basic_client_request &&other) noexcept;
	basic_client_request &operator=(basic_client_request &&other) noexcept;

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {}) requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto send_file (
		concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt,
		Token &&token = {}
	) requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(const map_helper_t &headers, Token &&token = {}) requires put_or_post;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto chunk_end(Token &&token = {}) requires put_or_post;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto wait_reply(Token &&token = {}) const requires
		core_concepts::tf_opt_token<error_code,reply_t>;

public:
	basic_client_request &set_arg(request_arg_t arg);
	[[nodiscard]] const request_arg_t &arg() const noexcept;
	[[nodiscard]] request_arg_t &arg() noexcept;

public:
	[[nodiscard]] bool is_finished() const noexcept;
	basic_client_request &cancel() noexcept;
	basic_client_request &reset() noexcept;

public:
	[[nodiscard]] consteval method_t method() const noexcept;
	[[nodiscard]] consteval version_t version() const noexcept;

	[[nodiscard]] const session_pool_t &session_pool() const noexcept;
	[[nodiscard]] session_pool_t &session_pool() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/client/detail/request.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_H