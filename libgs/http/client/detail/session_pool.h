
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H
#define LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H

#include <map>

namespace libgs::http
{

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
class basic_session_pool<Stream,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <core_concepts::match_execution<executor_t> Exec0>
	explicit impl(const Exec0 &exec) : m_exec(exec) {}
	impl() : m_exec(execution::get_executor()) {}

	~impl() {
		*m_valid = false;
	}

public:
	[[nodiscard]] session_t get(const endpoint_t &ep, auto &&exec)
	{
		socket_t socket(exec);
		auto it = m_sock_map.find(ep);

		if( it == m_sock_map.end() )
			socket = socket_t(exec);
		else
		{
			socket = std::move(it->second);
			m_sock_map.erase(it);
		}
		return session_t(std::move(socket), [this, valid = m_valid](socket_t &&sock)
		{
			if( *valid )
				emplace(std::move(sock));
		});
	}

	void emplace(socket_t &&socket)
	{
		if( socket.is_open() )
			m_sock_map.emplace(std::make_pair(socket.remote_endpoint(), std::move(socket)));
	}

public:
	std::shared_ptr<bool> m_valid {new bool(true)};
	std::map<endpoint_t,socket_t> m_sock_map;
	executor_t m_exec;
};

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(const core_concepts::match_execution<executor_t> auto &exec) :
	m_impl(new impl(exec))
{

}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(core_concepts::match_execution_context<executor_t> auto &context) :
	m_impl(new impl(context.get_executor()))
{

}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool() requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl())
{

}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::~basic_session_pool()
{
	delete m_impl;
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(basic_session_pool &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec> &basic_session_pool<Stream,Exec>::operator=(basic_session_pool &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
template <typename Token>
auto basic_session_pool<Stream,Exec>::get(const endpoint_t &ep, Token &&token)
	requires concepts::token<Token,session_t,error_code>
{
	return get(m_impl->m_exec, ep, std::forward<Token>(token));
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
template <typename Token>
auto basic_session_pool<Stream,Exec>::get
(const core_concepts::execution auto &exec, const endpoint_t &ep, Token &&token)
	requires concepts::token<Token,session_t,error_code>
{
	if constexpr( std::is_same_v<Token, error_code&> )
	{
		auto sess = m_impl->get(ep, exec);
		if( auto &helper = sess.opt_helper(); not helper.is_open() )
			helper.connect(ep, token);
		return sess;
	}
	else if constexpr( is_sync_token_v<Token> )
	{
		error_code error;
		auto sess = get(ep, error);
		if( error )
			throw system_error(error, "libgs::http::basic_session_pool::get");
		return sess;
	}
	else if constexpr( is_function_v<Token> )
	{
		auto sess = m_impl->get(ep, exec);
		if( auto &helper = sess.opt_helper(); helper.is_open() )
		{
			asio::dispatch(exec, [
				token = std::forward<Token>(token), sess = std::move(sess)
			]() mutable {
				token(std::move(sess), error_code());
			});
		}
		else
		{
			helper.connect(std::move(ep), [
				token = std::forward<Token>(token), sess = std::move(sess)
			](const error_code &error) mutable {
				token(std::move(sess), error);
			});
		}
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<Token> )
	{
		auto &helper = sess.opt_helper();
		if( not helper.is_open() )
			helper.connect(std::move(ep), std::forward<Token>(token));
		return std::move(sess);
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		auto sess = m_impl->get(ep, exec);
		return asio::co_spawn(exec, [
			ep = std::move(ep), sess = std::move(sess), token
		]() mutable -> awaitable<session_t>
		{
			if( auto &helper = sess.opt_helper(); not helper.is_open() )
				co_await helper.connect(std::move(ep), token);
			co_return std::move(sess);
		},
		token);
	}
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
template <typename Token>
auto basic_session_pool<Stream,Exec>::get
(core_concepts::execution_context auto &exec, const endpoint_t &ep, Token &&token)
	requires concepts::token<Token,session_t,error_code>
{
	return get(exec.get_executor(), ep, std::forward<Token>(token));
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
void basic_session_pool<Stream,Exec>::emplace(socket_t &&socket)
{
	m_impl->emplace(std::move(socket));
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
void basic_session_pool<Stream,Exec>::operator<<(socket_t &&socket)
{
	insert(std::move(socket));
}

template <concepts::any_exec_stream Stream, core_concepts::execution Exec>
typename basic_session_pool<Stream,Exec>::executor_t basic_session_pool<Stream,Exec>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H

