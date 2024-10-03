
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

namespace libgs::http
{

template <concepts::stream_requires Stream>
class basic_session_pool<Stream>::session::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using pool_t = basic_session_pool<Stream>;

public:
	impl() = default;
	~impl()
	{
		if( m_pool and m_socket.is_open() )
			m_pool->emplace(std::move(m_socket));
	}

public:
	pool_t *m_pool = nullptr;
	socket_t m_socket {execution::io_context()};
};

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::session::session() :
	m_impl(new impl())
{

}

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::session::~session()
{
	delete m_impl;
}

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::session::session(session &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::stream_requires Stream>
typename basic_session_pool<Stream>::session &basic_session_pool<Stream>::session::operator=(session &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::stream_requires Stream>
const typename basic_session_pool<Stream>::socket_t &basic_session_pool<Stream>::session::socket() const noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream_requires Stream>
typename basic_session_pool<Stream>::socket_t &basic_session_pool<Stream>::session::socket() noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream_requires Stream>
class basic_session_pool<Stream>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <core_concepts::match_execution<executor_t> Exec>
	explicit impl(Exec &exec) : m_exec(exec) {}

	template <core_concepts::match_execution_context<executor_t> Exec>
	explicit impl(Exec &exec) : m_exec(exec.get_executor()) {}

	impl() : m_exec(execution::io_context().get_executor()) {}

public:
	std::map<endpoint_t,socket_t> m_sock_map;
	executor_t m_exec;
};

template <concepts::stream_requires Stream>
template <core_concepts::match_execution_or_context<typename basic_session_pool<Stream>::executor_t> Exec>
basic_session_pool<Stream>::basic_session_pool(Exec &exec) :
	m_impl(new impl(exec))
{

}

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::basic_session_pool() requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl())
{

}

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::~basic_session_pool()
{
	delete m_impl;
}

template <concepts::stream_requires Stream>
basic_session_pool<Stream>::basic_session_pool(basic_session_pool &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::stream_requires Stream>
basic_session_pool<Stream> &basic_session_pool<Stream>::operator=(basic_session_pool &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::stream_requires Stream>
typename basic_session_pool<Stream>::session
basic_session_pool<Stream>::get(const endpoint_t &ep)
{
	return get(m_impl->m_exec, ep);
}

template <concepts::stream_requires Stream>
typename basic_session_pool<Stream>::session
basic_session_pool<Stream>::get(const endpoint_t &ep, error_code &error) noexcept
{
	return get(m_impl->m_exec, ep, error);
}

template <concepts::stream_requires Stream>
template <core_concepts::schedulable Exec>
basic_session_pool<Stream>::session basic_session_pool<Stream>::get(Exec &exec, const endpoint_t &ep)
{
	error_code error;
	auto sess = get(exec, ep, error);
	if( error )
          throw system_error(error, "libgs::http::basic_session_pool::get");
	return sess;
}

template <concepts::stream_requires Stream>
template <core_concepts::schedulable Exec>
basic_session_pool<Stream>::session basic_session_pool<Stream>::get(Exec &exec, const endpoint_t &ep, error_code &error) noexcept
{
	session sess;
	auto it = m_impl->m_sock_map.find(ep);

	if( it == m_impl->m_sock_map.end() )
		sess.m_impl->m_socket = socket_t(exec);
	else
	{
		sess.m_impl->m_socket = std::move(it->second);
		m_impl->m_sock_map.erase(it);
	}
	if(not sess.m_impl->m_socket.is_open())
		sess.m_impl->m_socket.connect(ep, error);
	return sess;
}

template <concepts::stream_requires Stream>
template <asio::completion_token_for<void(error_code)> Token>
auto basic_session_pool<Stream>::async_get(endpoint_t ep, session &sess, Token &&token)
{


	return 0;
}

template <concepts::stream_requires Stream>
template <core_concepts::schedulable Exec, asio::completion_token_for<void(error_code)> Token>
auto basic_session_pool<Stream>::async_get(Exec &exec, endpoint_t ep, session &sess, Token &&token)
{
	return 0;
}

template <concepts::stream_requires Stream>
void basic_session_pool<Stream>::emplace(socket_t &&socket)
{
	if( socket.is_open() )
		m_impl->m_sock_map.emplace(std::make_pair(socket.remote_endpoint(), std::move(socket)));
}

template <concepts::stream_requires Stream>
void basic_session_pool<Stream>::operator<<(socket_t &&socket)
{
	insert(std::move(socket));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H

