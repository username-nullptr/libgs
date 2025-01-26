
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

template <concepts::stream Stream, core_concepts::execution Exec>
class LIBGS_HTTP_TAPI basic_session_pool<Stream,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <core_concepts::match_execution<executor_t> Exec0>
	explicit impl(const Exec0 &exec) : m_exec(exec) {}
	impl() : m_exec(libgs::get_executor()) {}

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
		return session_t(std::move(socket), [this, valid = m_valid](socket_t &&sock) mutable
		{
			dispatch(m_exec, [this, valid = std::move(valid), sock = std::move(sock)]() mutable
			{
				if( *valid )
					emplace(std::move(sock));
			});
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

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(const core_concepts::match_execution<executor_t> auto &exec) :
	m_impl(new impl(exec))
{

}

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(core_concepts::match_execution_context<executor_t> auto &context) :
	m_impl(new impl(context.get_executor()))
{

}

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool() requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl())
{

}

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::~basic_session_pool()
{
	delete m_impl;
}

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec>::basic_session_pool(basic_session_pool &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::stream Stream, core_concepts::execution Exec>
basic_session_pool<Stream,Exec> &basic_session_pool<Stream,Exec>::operator=(basic_session_pool &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::stream Stream, core_concepts::execution Exec>
template <typename Token>
auto basic_session_pool<Stream,Exec>::get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,session_t>
{
	return get(m_impl->m_exec, ep, std::forward<Token>(token));
}

template <concepts::stream Stream, core_concepts::execution Exec>
template <typename Token>
auto basic_session_pool<Stream,Exec>::get
(core_concepts::match_execution_or_context<socket_executor_t> auto &&exec, const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,session_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code&> )
	{
		auto sess = m_impl->get(ep, exec);
		if( auto &helper = sess.opt_helper(); not helper.is_open() )
			helper.connect(ep, token);
		return sess;
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto sess = get(ep, error);
		if( error )
			throw system_error(error, "libgs::http::basic_session_pool::get");
		return sess;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		if( auto &helper = sess.opt_helper(); not helper.is_open() )
			helper.connect(std::move(ep), std::forward<Token>(token));
		return std::move(sess);
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;

		decltype(auto) ntoken = unbound_redirect_time(token);
		decltype(auto) rtoken = unbound_token(ntoken);

		return async_work<error_code,session_t>::handle(get_executor(), [
			self_exec = get_executor(), ep = std::move(ep), sess = m_impl->get(ep, exec),
			timeout = get_associated_redirect_time(token), ntoken = std::move(ntoken)
		](auto wake_up) mutable
		{
			using wake_up_t = std::remove_cvref_t<decltype(wake_up)>;
			asio::co_spawn(self_exec, [
				wake_up = std::make_shared<wake_up_t>(std::move(wake_up)), ep = std::move(ep),
				sess = std::move(sess), timeout = std::move(timeout), ntoken = std::move(ntoken)
			]() mutable -> awaitable<void>
			{
				auto &helper = sess.opt_helper();
				std::error_code error;

				if( helper.is_open() )
					std::move(*wake_up)(error, std::move(sess));

				auto var = co_await (
					helper.connect(std::move(ep), use_awaitable | error) or
					co_sleep_for(timeout /*,get_executor()*/)
				);
				if( var.index() == 1 and not error )
					error = make_error_code(errc::timed_out);

				std::move(*wake_up)(error, std::move(sess));
				co_return ;
			},
			detached | asio::get_associated_cancellation_slot(ntoken));
		},
		rtoken);
	}
}

template <concepts::stream Stream, core_concepts::execution Exec>
void basic_session_pool<Stream,Exec>::emplace(socket_t &&socket)
{
	m_impl->emplace(std::move(socket));
}

template <concepts::stream Stream, core_concepts::execution Exec>
void basic_session_pool<Stream,Exec>::operator<<(socket_t &&socket)
{
	emplace(std::move(socket));
}

template <concepts::stream Stream, core_concepts::execution Exec>
typename basic_session_pool<Stream,Exec>::executor_t basic_session_pool<Stream,Exec>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H

