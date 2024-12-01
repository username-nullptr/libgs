
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

#ifndef LIBGS_HTTP_CXX_DETAIL_SOCKET_SESSION_H
#define LIBGS_HTTP_CXX_DETAIL_SOCKET_SESSION_H

namespace libgs::http
{

template <concepts::stream Stream>
class basic_socket_session<Stream>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
  	template <core_concepts::callable<socket_t&&> Func>
	impl(socket_t &&socket, Func &&destructor) :
		m_destructor(std::forward<Func>(destructor)),
		m_socket(std::move(socket)) {}

	explicit impl(socket_t &&socket) :
		m_socket(std::move(socket)) {}

	impl() = default;
	~impl()
	{
  		if( m_destructor )
			m_destructor(std::move(m_socket));
	}

public:
	std::function<void(socket_t&&)> m_destructor {};
	socket_t m_socket {execution::get_executor()};
	opt_helper_t m_opt_helper {m_socket};
};

template <concepts::stream Stream>
template <typename Func>
basic_socket_session<Stream>::basic_socket_session(socket_t &&socket, Func &&destructor)
	requires core_concepts::callable<Func,socket_t&&> :
	m_impl(new impl(std::move(socket), std::forward<Func>(destructor)))
{

}

template <concepts::stream Stream>
basic_socket_session<Stream>::basic_socket_session(socket_t &&socket) :
	m_impl(new impl(std::move(socket)))
{

}

template <concepts::stream Stream>
basic_socket_session<Stream>::basic_socket_session() :
	m_impl(new impl())
{

}

template <concepts::stream Stream>
basic_socket_session<Stream>::~basic_socket_session()
{
	delete m_impl;
}

template <concepts::stream Stream>
basic_socket_session<Stream>::basic_socket_session(basic_socket_session &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::stream Stream>
basic_socket_session<Stream> &basic_socket_session<Stream>::operator=(basic_socket_session &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl({},{});
	return *this;
}

template <concepts::stream Stream>
basic_socket_session<Stream> &basic_socket_session<Stream>::operator=(socket_t &&socket) noexcept
{
	m_impl->m_socket = std::move(socket);
	return *this;
}

template <concepts::stream Stream>
const typename basic_socket_session<Stream>::socket_t&
basic_socket_session<Stream>::socket() const noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream Stream>
typename basic_socket_session<Stream>::socket_t&
basic_socket_session<Stream>::socket() noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream Stream>
const typename basic_socket_session<Stream>::opt_helper_t&
basic_socket_session<Stream>::opt_helper() const noexcept
{
	return m_impl->m_opt_helper;
}

template <concepts::stream Stream>
typename basic_socket_session<Stream>::opt_helper_t&
basic_socket_session<Stream>::opt_helper() noexcept
{
	return m_impl->m_opt_helper;
}

template <concepts::stream Stream>
typename basic_socket_session<Stream>::executor_t
basic_socket_session<Stream>::get_executor() noexcept
{
	return m_impl->m_socket.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_SOCKET_SESSION_H
