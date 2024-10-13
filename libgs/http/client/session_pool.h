
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

#ifndef LIBGS_HTTP_CLIENT_SESSION_POOL_H
#define LIBGS_HTTP_CLIENT_SESSION_POOL_H

#include <libgs/http/cxx/socket_session.h>
#include <libgs/core/execution.h>

namespace libgs::http
{

template <concepts::stream_requires Stream, core_concepts::execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_session_pool
{
	LIBGS_DISABLE_COPY(basic_session_pool)

public:
	using socket_t = Stream;
	using session_t = basic_socket_session<socket_t>;

	using executor_t = Exec;
	using endpoint_t = typename session_t::endpoint_t;

public:
	explicit basic_session_pool(const core_concepts::match_execution<executor_t> auto &exec);
	explicit basic_session_pool(core_concepts::match_execution_context<executor_t> auto &context);

	basic_session_pool() requires core_concepts::match_default_execution<executor_t>;
	~basic_session_pool();

	basic_session_pool(basic_session_pool &&other) noexcept;
	basic_session_pool &operator=(basic_session_pool &&other) noexcept;

public:
	[[nodiscard]] session_t get(const endpoint_t &ep);
	[[nodiscard]] session_t get(const endpoint_t &ep, error_code &error) noexcept;

	[[nodiscard]] session_t get
	(const core_concepts::execution auto &exec, const endpoint_t &ep);

	[[nodiscard]] session_t get
	(core_concepts::execution_context auto &exec, const endpoint_t &ep);

	[[nodiscard]] session_t get
	(const core_concepts::execution auto &exec, const endpoint_t &ep, error_code &error) noexcept;

	[[nodiscard]] session_t get
	(core_concepts::execution_context auto &exec, const endpoint_t &ep, error_code &error) noexcept;

	template <typename Token>
	[[nodiscard]] auto async_get(endpoint_t ep, Token &&token)
		requires asio::completion_token_for<Token,void(session_t,error_code)>;

	template <typename Token>
	[[nodiscard]] auto async_get(const core_concepts::execution auto &exec, endpoint_t ep, Token &&token)
		requires asio::completion_token_for<Token,void(session_t,error_code)>;

	template <typename Token>
	[[nodiscard]] auto async_get(core_concepts::execution_context auto &exec, endpoint_t ep, Token &&token)
		requires asio::completion_token_for<Token,void(session_t,error_code)>;

public:
	void emplace(socket_t &&socket);
	void operator<<(socket_t &&socket);

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution MainExec, core_concepts::execution SockExec>
using basic_tcp_session_pool = basic_session_pool<asio::basic_stream_socket<asio::ip::tcp,SockExec>, MainExec>;

template <core_concepts::execution Exec>
using tcp_session_pool = basic_tcp_session_pool<asio::any_io_executor, Exec>;

using session_pool = tcp_session_pool<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/client/detail/session_pool.h>


#endif //LIBGS_HTTP_CLIENT_SESSION_POOL_H
