
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

#include <libgs/http/basic/opt_token.h>

namespace libgs::http
{

template <concepts::stream_requires Stream>
class LIBGS_HTTP_TAPI basic_session_pool
{
	LIBGS_DISABLE_COPY(basic_session_pool)

public:
	using socket_t = Stream;
	using executor_t = typename socket_t::executor_type;
	using endpoint_t = typename socket_t::endpoint_type;

public:
	template <core_concepts::match_execution_or_context<executor_t> Exec>
	explicit basic_session_pool(Exec &exec);
	~basic_session_pool();

	basic_session_pool(basic_session_pool &&other) noexcept;
	basic_session_pool &operator=(basic_session_pool &&other) noexcept;

public:
	class session
	{
		LIBGS_DISABLE_COPY(session)
		friend class basic_session_pool;
		
	public:
		session();
		~session();

		session(session &&other) noexcept;
		session &operator=(session &&other) noexcept;

	public:
		const socket_t &socket() const noexcept;
		socket_t &socket() noexcept;

	private:
		class impl;
		impl *m_impl;
	};

public:
	[[nodiscard]] session get(const endpoint_t &ep);
	[[nodiscard]] session get(const endpoint_t &ep, error_code &error) noexcept;

	template <core_concepts::schedulable Exec>
	[[nodiscard]] session get(Exec &exec, const endpoint_t &ep);

	template <core_concepts::schedulable Exec>
	[[nodiscard]] session get(Exec &exec, const endpoint_t &ep, error_code &error) noexcept;

	template <asio::completion_token_for<void(error_code)> Token>
	void async_get(endpoint_t ep, session &sess, Token &&token);

	template <core_concepts::schedulable Exec, asio::completion_token_for<void(error_code)> Token>
	void async_get(Exec &exec, endpoint_t ep, session &sess, Token &&token);

public:
	void emplace(socket_t &&socket);
	void operator<<(socket_t &&socket);

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec = asio::any_io_executor>
using tcp_session_pool = basic_session_pool<asio::basic_stream_socket<asio::ip::tcp,Exec>>;

using session_pool = tcp_session_pool<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/client/detail/session_pool.h>


#endif //LIBGS_HTTP_CLIENT_SESSION_POOL_H

