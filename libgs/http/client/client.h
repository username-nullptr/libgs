
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

#ifndef LIBGS_HTTP_CLIENT_CLIENT_H
#define LIBGS_HTTP_CLIENT_CLIENT_H

#include <libgs/http/client/session_pool.h>
#include <libgs/http/client/reply.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, concepts::session_pool SessionPool = session_pool>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using char_t = CharT;
	using session_pool_t = SessionPool;

	using socket_t = typename session_pool_t::socket_t;
	using executor_t = typename session_pool_t::executor_t;

	using request_t = basic_client_request<char_t>;
	using url_t = typename request_t::url_t;

    using get_body_t = std::function<std::string()>;
	using reply_t = basic_client_reply<char_t>;

public:
	explicit basic_client(const core_concepts::match_execution<executor_t> auto &exec);
	explicit basic_client(core_concepts::match_execution_context<executor_t> auto &context);

	basic_client() requires core_concepts::match_default_execution<executor_t>;
	~basic_client();

	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;

public:
	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto get(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto put(request_t request, const const_buffer &body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto put(request_t request, const get_body_t &get_body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto put(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto post(request_t request, const const_buffer &body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto post(request_t request, const get_body_t &get_body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto post(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto Delete(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto head(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto options(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto trace(request_t request, Token &&token = {});

public:
	template <method_t Method, core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(request_t request, const const_buffer &body, Token &&token = {})
    	requires (Method == method_t::POST || Method == method_t::PUT);

	template <method_t Method, core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(request_t request, const get_body_t &get_body, Token &&token = {})
    	requires (Method == method_t::POST || Method == method_t::PUT);

	template <method_t Method, core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(request_t request, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(method_t method, request_t request, const const_buffer &body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(method_t method, request_t request, const get_body_t &get_body, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	[[nodiscard]] auto request(method_t method, request_t request, Token &&token = {});

public:
	[[nodiscard]] const session_pool_t &session_pool() const noexcept;
	[[nodiscard]] session_pool_t &session_pool() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};




} //namespace libgs::http
#include <libgs/http/client/detail/client.h>

#ifdef LIBGS_ENABLE_OPENSSL
// TODO ... ...
#endif //LIBGS_ENABLE_OPENSSL


#endif //LIBGS_HTTP_CLIENT_CLIENT_H
