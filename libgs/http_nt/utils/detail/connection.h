
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_UTILS_DETAIL_CONNECTION_H
#define LIBGS_HTTP_NT_UTILS_DETAIL_CONNECTION_H

namespace libgs::http_nt
{

template <concepts::stream Stream>
class LIBGS_HTTP_NT_TAPI basic_connection<Stream>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	template <core_concepts::callable<socket_t&&> Func>
	impl(socket_t &&socket, Func &&destructor) :
		m_destructor(std::forward<Func>(destructor)),
		m_socket(std::move(socket)) {}

	explicit impl(socket_t &&socket) :
		m_socket(std::move(socket)) {}

	impl(impl &&other) noexcept :
		m_destructor(std::move(other.m_destructor)),
		m_socket(std::move(other.m_socket))
	{
		other.m_opt_helper.close();
	}

	impl &operator=(impl &&other) noexcept
	{
		m_destructor = std::move(other.m_destructor);
		m_socket = std::move(other.m_socket);
		other.m_opt_helper.close();
		return *this;
	}

	~impl()
	{
		if( m_destructor )
		{
			opt_helper_t(m_socket).cancel();
			m_destructor(std::move(m_socket));
		}
	}

public:
	std::function<void(socket_t&&)> m_destructor {};
	socket_t m_socket;
	opt_helper_t m_opt_helper {m_socket};
};

template <concepts::stream Stream>
template <typename Func>
basic_connection<Stream>::basic_connection(socket_t &&socket, Func &&destructor)
	requires core_concepts::callable<Func,socket_t&&> :
	m_impl(new impl(std::move(socket), std::forward<Func>(destructor)))
{

}

template <concepts::stream Stream>
basic_connection<Stream>::basic_connection(socket_t &&socket) :
	m_impl(new impl(std::move(socket)))
{

}

template <concepts::stream Stream>
basic_connection<Stream>::~basic_connection()
{
	delete m_impl;
}

template <concepts::stream Stream>
basic_connection<Stream>::basic_connection(basic_connection &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::stream Stream>
basic_connection<Stream> &basic_connection<Stream>::operator=(basic_connection &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream>
basic_connection<Stream> &basic_connection<Stream>::operator=(socket_t &&socket) noexcept
{
	m_impl->m_socket = std::move(socket);
	return *this;
}

template <concepts::stream Stream>
const basic_connection<Stream>::socket_t&
basic_connection<Stream>::socket() const noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream Stream>
basic_connection<Stream>::socket_t&
basic_connection<Stream>::socket() noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream Stream>
const basic_connection<Stream>::opt_helper_t&
basic_connection<Stream>::opt_helper() const noexcept
{
	return m_impl->m_opt_helper;
}

template <concepts::stream Stream>
basic_connection<Stream>::opt_helper_t&
basic_connection<Stream>::opt_helper() noexcept
{
	return m_impl->m_opt_helper;
}

template <concepts::stream Stream>
bool basic_connection<Stream>::peek() noexcept
{
	return opt_helper().is_open() and opt_helper().message_peek();
}

template <concepts::stream Stream>
basic_connection<Stream>::executor_t
basic_connection<Stream>::get_executor() noexcept
{
	return m_impl->m_socket.get_executor();
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_UTILS_DETAIL_CONNECTION_H
