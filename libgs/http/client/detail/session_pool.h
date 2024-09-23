
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

template <concept_stream_requires Stream>
class basic_session_pool<Stream>::session::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using pool_t = basic_session_pool<Stream>;

public:
	impl() = default;
	~impl()
	{
		if( m_pool and m_socket.is_open() )
			m_pool->insert(std::move(m_socket));
	}

public:
	pool_t *m_pool = nullptr;
	socket_t m_socket;
};

template <concept_stream_requires Stream>
basic_session_pool<Stream>::session::session() :
	m_impl(new impl())
{

}

template <concept_stream_requires Stream>
basic_session_pool<Stream>::session::~session()
{
	delete m_impl;
}

template <concept_stream_requires Stream>
basic_session_pool<Stream>::session::session(session &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_stream_requires Stream>
typename basic_session_pool<Stream>::session &basic_session_pool<Stream>::session::operator=(session &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_stream_requires Stream>
const typename basic_session_pool<Stream>::socket_t &basic_session_pool<Stream>::session::socket() const noexcept
{
	return m_impl->m_socket;
}

template <concept_stream_requires Stream>
typename basic_session_pool<Stream>::socket_t &basic_session_pool<Stream>::session::socket() noexcept
{
	return m_impl->m_socket;
}

template <concept_stream_requires Stream>
class basic_session_pool<Stream>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;
	std::map<endpoint_t, socket_t> m_sock_map;
};

template <concept_stream_requires Stream>
basic_session_pool<Stream>::basic_session_pool() :
	m_impl(new impl())
{

}

template <concept_stream_requires Stream>
basic_session_pool<Stream>::~basic_session_pool()
{
	delete m_impl;
}

template <concept_stream_requires Stream>
basic_session_pool<Stream>::basic_session_pool(basic_session_pool &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_stream_requires Stream>
basic_session_pool<Stream> &basic_session_pool<Stream>::operator=(basic_session_pool &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_stream_requires Stream>
typename basic_session_pool<Stream>::session basic_session_pool<Stream>::get(endpoint_t ep)
{

}

template <concept_stream_requires Stream>
typename basic_session_pool<Stream>::session basic_session_pool<Stream>::get(endpoint_t ep, error_code &error) noexcept
{

}

template <concept_stream_requires Stream>
template <asio::completion_token_for<void(error_code)> Token>
void basic_session_pool<Stream>::async_get(endpoint_t ep, session &sess, Token &&token)
{

}

template <concept_stream_requires Stream>
void basic_session_pool<Stream>::insert(socket_t &&socket)
{
	if( socket.is_open() )
		m_impl->m_sock_map.emplate(std::make_pair(socket.remote_endpoint(), std::move(socket)));
}

template <concept_stream_requires Stream>
void basic_session_pool<Stream>::operator<<(socket_t &&socket)
{
	insert(std::move(socket));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_SESSION_POOL_H

