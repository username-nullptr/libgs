
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
#include <libgs/http/client/request.h>

namespace libgs::http
{

template <core_concepts::char_type CharT,
		  concepts::session_pool SessionPool = session_pool,
		  version_t Version = version::v11>
class LIBGS_HTTP_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using char_t = CharT;
	using session_pool_t = SessionPool;
	using string_view_t = std::basic_string_view<char_t>;

	using socket_t = typename session_pool_t::socket_t;
	using executor_t = typename session_pool_t::executor_t;

	static constexpr auto version_v = Version;

	template <method Method>
	using request_t = basic_client_request <
		char_t, Method, session_pool_t, version_v
	>;
	using request_arg_t = basic_request_arg<char_t>;
	using url_t = typename request_arg_t::url_t;

public:
	explicit basic_client(const core_concepts::match_execution<executor_t> auto &exec);
	explicit basic_client(core_concepts::match_execution_context<executor_t> auto &context);
	basic_client() requires core_concepts::match_default_execution<executor_t>;

	~basic_client();
	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;

public:
	template <method Method, core_concepts::tf_opt_token<error_code,request_t<Method>> Token = use_sync_t>
	[[nodiscard]] auto request(request_arg_t arg, Token &&token = {});

	template <method Method, core_concepts::tf_opt_token<error_code,request_t<Method>> Token = use_sync_t>
	[[nodiscard]] auto request(request_arg_t arg, const const_buffer &body, Token &&token = {})
		requires (Method == method::POST or Method == method::PUT);

	template <method Method, core_concepts::tf_opt_token<error_code,request_t<Method>> Token = use_sync_t>
	[[nodiscard]] auto request(request_arg_t arg,
		concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt,
		Token &&token = {}
	) requires (Method == method::POST or Method == method::PUT);

public:
	template <core_concepts::tf_opt_token<error_code,request_t<method::GET>> Token = use_sync_t>
	[[nodiscard]] auto get(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::POST>> Token = use_sync_t>
	[[nodiscard]] auto post(request_arg_t arg, const const_buffer &body, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::POST>> Token = use_sync_t>
	[[nodiscard]] auto post(request_arg_t arg,
		concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt,
		Token &&token = {}
	);

	template <core_concepts::tf_opt_token<error_code,request_t<method::PUT>> Token = use_sync_t>
	[[nodiscard]] auto put(request_arg_t arg, const const_buffer &body, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::PUT>> Token = use_sync_t>
	[[nodiscard]] auto put(request_arg_t arg,
		concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt,
		Token &&token = {}
	);

	template <core_concepts::tf_opt_token<error_code,request_t<method::HEAD>> Token = use_sync_t>
	[[nodiscard]] auto head(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::PATCH>> Token = use_sync_t>
	[[nodiscard]] auto patch(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::DELETE>> Token = use_sync_t>
	[[nodiscard]] auto Delete(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::OPTIONS>> Token = use_sync_t>
	[[nodiscard]] auto options(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::TRACE>> Token = use_sync_t>
	[[nodiscard]] auto trace(request_arg_t arg, Token &&token = {});

	template <core_concepts::tf_opt_token<error_code,request_t<method::CONNECT>> Token = use_sync_t>
	[[nodiscard]] auto connect(request_arg_t arg, Token &&token = {});

public:
	template <method Method>
	[[nodiscard]] request_t<Method> make_request(request_arg_t arg);

	[[nodiscard]] request_t<method::GET> make_get(request_arg_t arg);
	[[nodiscard]] request_t<method::POST> make_post(request_arg_t arg);
	[[nodiscard]] request_t<method::PUT> make_put(request_arg_t arg);
	[[nodiscard]] request_t<method::HEAD> make_head(request_arg_t arg);
	[[nodiscard]] request_t<method::PATCH> make_patch(request_arg_t arg);
	[[nodiscard]] request_t<method::DELETE> make_delete(request_arg_t arg);
	[[nodiscard]] request_t<method::OPTIONS> make_options(request_arg_t arg);
	[[nodiscard]] request_t<method::TRACE> make_trace(request_arg_t arg);
	[[nodiscard]] request_t<method::CONNECT> make_connect(request_arg_t arg);

public:
	[[nodiscard]] consteval version_t version() const noexcept;
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
