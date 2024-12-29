
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

/*
	  client -------- request_url
	    |				  |
	session_pool ----- context --- cilent_request
                          |
                      client_reply
*/
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

	using reply_t = typename request_t::reply_t;
	using url_t = typename request_t::url_t;

public:
	explicit basic_client(const core_concepts::match_execution<executor_t> auto &exec);
	explicit basic_client(core_concepts::match_execution_context<executor_t> auto &context);

	basic_client() requires core_concepts::match_default_execution<executor_t>;
	~basic_client();

	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;

public:
	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto get(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto put(const const_buffer &buf, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto put(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto post(const const_buffer &buf, Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto post(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto Delete(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto head(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto options(Token &&token = {});

	template <core_concepts::tf_opt_token Token = use_sync_t>
	auto trach(Token &&token = {});

public:
	template <method_t Method, core_concepts::tf_opt_token Token = use_sync_t>
	auto request(const const_buffer &buf, Token &&token = {});

	template <method_t Method, core_concepts::tf_opt_token Token = use_sync_t>
	auto request(Token &&token = {});

public:
	[[nodiscard]] const session_pool_t &session_pool() const noexcept;
	[[nodiscard]] session_pool_t &session_pool() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec, core_concepts::execution StreamExec = asio::any_io_executor>
using basic_tcp_client = basic_client<char, asio::basic_stream_socket<asio::ip::tcp,StreamExec>, Exec>;

template <core_concepts::execution Exec, core_concepts::execution StreamExec = asio::any_io_executor>
using wbasic_tcp_client = basic_client<wchar_t, asio::basic_stream_socket<asio::ip::tcp,StreamExec>, Exec>;

using tcp_client = basic_tcp_client<asio::any_io_executor>;
using wtcp_client = wbasic_tcp_client<asio::any_io_executor>;

using client = tcp_client;
using wclient = wtcp_client;

} //namespace libgs::http
#include <libgs/http/client/detail/client.h>

#ifdef LIBGS_ENABLE_OPENSSL
// TODO ... ...
#endif //LIBGS_ENABLE_OPENSSL


#endif //LIBGS_HTTP_CLIENT_CLIENT_H
