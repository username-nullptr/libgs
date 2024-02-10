
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

#include "tcp_socket.h"
#include "detail/waitable.h"

namespace libgs::io
{

tcp_socket::tcp_socket(asio::io_context &io) :
	m_sock(new asio_tcp_socket(io))
{
	
}

tcp_socket::tcp_socket(asio_tcp_socket &&sock) :
	m_sock(new asio_tcp_socket(std::move(sock)))
{

}

tcp_socket::~tcp_socket() 
{

}

void tcp_socket::async_connect(const address_t &addr, port_t port, callback_t callback) noexcept
{
	m_sock->async_connect({addr, port}, std::move(callback));
}

error_code tcp_socket::connect(const address_t &addr, port_t port) noexcept 
{
	error_code error;
	m_sock->connect({addr, port}, error);
	return error;
}

tcp_socket::await_error_t tcp_socket::co_connect(const address_t &addr, port_t port) noexcept
{
	error_code error;
	co_await m_sock->async_connect({addr, port}, use_awaitable_e[error]);
	co_return error;
}

void tcp_socket::async_read_some(void *buf, size_t size, rw_callback_t callback) noexcept 
{
	m_sock->async_read_some(asio::buffer(buf, size), [callback = std::move(callback)](error_code error, size_t size){
		callback(size, error);
	});
}

size_t tcp_socket::read_some(void *buf, size_t size, error_code *error) noexcept 
{
	m_sock->non_blocking(false);
	return m_sock->read_some(asio::buffer(buf, size));
}

tcp_socket::await_size_t tcp_socket::co_read_some(void *buf, size_t size, error_code *error) noexcept
{
	error_code _error;
	auto res = co_await m_sock->async_read_some(asio::buffer(buf, size), use_awaitable_e[_error]);
	if( error )
		*error = _error;
	co_return res;
}

void tcp_socket::async_write_some(const void *buf, size_t size, rw_callback_t callback) noexcept 
{
	m_sock->async_write_some(asio::buffer(buf, size), [callback = std::move(callback)](error_code error, size_t size){
		callback(size, error);
	});
}

size_t tcp_socket::write_some(const void *buf, size_t size, error_code *error) noexcept 
{
	m_sock->non_blocking(false);
	return m_sock->write_some(asio::buffer(buf, size));
}

tcp_socket::await_size_t tcp_socket::co_write_some(const void *buf, size_t size, error_code *error) noexcept
{
	error_code _error;
	auto res = co_await m_sock->async_write_some(asio::buffer(buf, size), use_awaitable_e[_error]);
	if( error )
		*error = _error;
	co_return res;
}

tcp_socket::address_t tcp_socket::remote_address(error_code *error) const noexcept
{
	error_code _error;
	auto endpoint = m_sock->remote_endpoint(_error);
	if( error )
		*error = _error;
	return endpoint.address();
}

tcp_socket::port_t tcp_socket::remote_port(error_code *error) const noexcept
{
	error_code _error;
	auto endpoint = m_sock->remote_endpoint(_error);
	if( error )
		*error = _error;
	return endpoint.port();
}

tcp_socket::address_t tcp_socket::local_address(error_code *error) const noexcept
{
	error_code _error;
	auto endpoint = m_sock->local_endpoint(_error);
	if( error )
		*error = _error;
	return endpoint.address();
}

tcp_socket::port_t tcp_socket::local_port(error_code *error) const noexcept
{
	error_code _error;
	auto endpoint = m_sock->local_endpoint(_error);
	if( error )
		*error = _error;
	return endpoint.port();
}

error_code tcp_socket::shutdown(shutdown_type what) noexcept 
{
	error_code error;
	m_sock->shutdown(what, error);
	return error;
}

error_code tcp_socket::close() noexcept
{
	error_code error;
	m_sock->close(error);
	return error;
}

bool tcp_socket::is_open() const noexcept
{
	return m_sock->is_open();
}

bool tcp_socket::wait_writeable(const duration &ms, error_code *error) noexcept
{
	return detail::wait_writeable(m_sock->native_handle(), ms, error);
}

bool tcp_socket::wait_readable(const duration &ms, error_code *error) noexcept
{
	return detail::wait_readable(m_sock->native_handle(), ms, error);
}

} //namespace libgs::io
