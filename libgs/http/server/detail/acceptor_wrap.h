
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_ACCEPTOR_WRAP_H
#define LIBGS_HTTP_SERVER_DETAIL_ACCEPTOR_WRAP_H

#include <libgs/http/basic/socket_operation_helper.h>
#include <spdlog/spdlog.h>

namespace libgs::http { namespace detail
{

template <core_concepts::execution Exec>
acceptor_wrap<Exec>::acceptor_wrap(acceptor_t &&acceptor) :
	m_acceptor(std::move(acceptor))
{

}

template <core_concepts::execution Exec>
const typename acceptor_wrap<Exec>::acceptor_t &acceptor_wrap<Exec>::acceptor() const
{
	return m_acceptor;
}

template <core_concepts::execution Exec>
typename acceptor_wrap<Exec>::acceptor_t &acceptor_wrap<Exec>::acceptor()
{
	return m_acceptor;
}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
acceptor_wrap<Exec>::acceptor_wrap(acceptor_wrap<Exec0> &&other) noexcept :
	m_acceptor(std::move(other.m_acceptor))
{

}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
acceptor_wrap<Exec> &acceptor_wrap<Exec>::operator=(acceptor_wrap<Exec0> &&other) noexcept
{
	m_acceptor = std::move(other.m_acceptor);
	return *this;
}

} //namespace detail

template <core_concepts::execution Exec>
basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>::basic_acceptor_wrap(acceptor_t &&acceptor) :
	base_t(std::move(acceptor))
{

}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>::basic_acceptor_wrap
(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept :
	base_t(std::move(other.m_acceptor))
{

}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>&
basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>::operator=
(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept
{
	base_t::operator=(std::move(other.m_acceptor));
	return *this;
}

template <core_concepts::execution Exec>
awaitable<typename basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>::socket_t>
basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>>::accept(core_concepts::execution auto &service_exec)
{
	co_return co_await this->m_acceptor.async_accept(service_exec, use_awaitable);
}

#ifdef LIBGS_ENABLE_OPENSSL

template <core_concepts::execution Exec>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
basic_acceptor_wrap(acceptor_t &&acceptor, asio::ssl::context &ssl) :
	base_t(std::move(acceptor))
{
	m_ssl = &ssl;
}

template <core_concepts::execution Exec>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::basic_acceptor_wrap
(basic_acceptor_wrap &&other) noexcept :
	base_t(std::move(other.m_acceptor))
{
	m_ssl = other.m_ssl;
}

template <core_concepts::execution Exec>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>&
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::operator=
(basic_acceptor_wrap &&other) noexcept
{
	base_t::operator=(std::move(other.m_acceptor));
	m_ssl = other.other.m_ssl;
	return *this;
}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::basic_acceptor_wrap
(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept :
	base_t(std::move(other.m_acceptor))
{
	m_ssl = other.m_ssl;
}

template <core_concepts::execution Exec>
template <core_concepts::execution Exec0>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>&
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::operator=
	(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept
{
	base_t::operator=(std::move(other.m_acceptor));
	m_ssl = other.other.m_ssl;
	return *this;
}

template <core_concepts::execution Exec>
awaitable<typename basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::socket_t>
basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::accept(core_concepts::execution auto &service_exec)
{
	auto tcp_socket = co_await this->m_acceptor.async_accept(service_exec, use_awaitable);
	socket_t ssl_socket(std::move(tcp_socket), *m_ssl);

	error_code error;
	co_await ssl_socket.async_handshake(asio::ssl::stream_base::server, use_awaitable|error);
	if( error )
	{
		spdlog::warn("libgs::http::server(SSL): SSL handshake failed: {}.", error);
		socket_operation_helper<socket_t>::close(ssl_socket);
	}
	co_return ssl_socket;
}

#endif //LIBGS_ENABLE_OPENSSL

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_ACCEPTOR_WRAP_H
