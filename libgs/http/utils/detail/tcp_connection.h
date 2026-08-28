
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_UTILS_DETAIL_TCP_CONNECTION_H
#define LIBGS_HTTP_UTILS_DETAIL_TCP_CONNECTION_H

namespace libgs::http
{

template <core_concepts::exec Exec>
basic_tcp_connection<Exec>::basic_tcp_connection(socket_t &&socket) :
	m_socket(std::move(socket))
{

}

template <core_concepts::exec Exec>
basic_tcp_connection<Exec>::~basic_tcp_connection() = default;

template <core_concepts::exec Exec>
sys_expected<> basic_tcp_connection<Exec>::cancel() noexcept
{
	if( not m_socket.is_open() )
		return make_sys_expected();
	error_code error {};
	m_socket.cancel(error);
	if( error )
		return sys_unexpected(error);
	return make_sys_expected();
}

template <core_concepts::exec Exec>
sys_expected<> basic_tcp_connection<Exec>::close() noexcept
{
	if( not m_socket.is_open() )
		return make_sys_expected();

	error_code ignored {};
	m_socket.cancel(ignored);
	m_socket.shutdown(asio::socket_base::shutdown_both, ignored);

	error_code error {};
	m_socket.close(error);
	if( error )
		return sys_unexpected(error);
	return make_sys_expected();
}

template <core_concepts::exec Exec>
sys_expected<> basic_tcp_connection<Exec>::set_options(const tcp_socket_options &options) noexcept
{
	return detail::set_tcp_socket_options(m_socket, options);
}

template <core_concepts::exec Exec>
sys_expected<tcp_socket_state>
basic_tcp_connection<Exec>::options() const noexcept
{
	return detail::get_tcp_socket_options(m_socket);
}

template <core_concepts::exec Exec>
bool basic_tcp_connection<Exec>::is_open() const noexcept
{
	return m_socket.is_open();
}

template <core_concepts::exec Exec>
sys_expected<typename basic_tcp_connection<Exec>::probe_state_t>
basic_tcp_connection<Exec>::probe() noexcept
{
	return detail::probe_tcp_socket(m_socket);
}

template <core_concepts::exec Exec>
endpoint basic_tcp_connection<Exec>::remote_endpoint() const noexcept
{
	error_code error {};
	auto value = m_socket.remote_endpoint(error);
	return error ? endpoint{} : detail::to_endpoint(value);
}

template <core_concepts::exec Exec>
endpoint basic_tcp_connection<Exec>::local_endpoint() const noexcept
{
	error_code error {};
	auto value = m_socket.local_endpoint(error);
	return error ? endpoint{} : detail::to_endpoint(value);
}

template <core_concepts::exec Exec>
basic_tcp_connection<Exec>::executor_t basic_tcp_connection<Exec>::get_executor() noexcept
{
	return m_socket.get_executor();
}

template <core_concepts::exec Exec>
io_expected basic_tcp_connection<Exec>::read_some(mutable_buffer buffer) noexcept
{
	error_code error {};
	auto size = m_socket.read_some(buffer, error);
	if( error )
		return io_unexpected(error);
	return size;
}

template <core_concepts::exec Exec>
io_expected basic_tcp_connection<Exec>::write_all(const const_buffer &buffer) noexcept
{
	error_code error {};
	auto size = asio::write(m_socket, buffer, error);
	if( error )
		return io_unexpected(error);
	return size;
}

template <core_concepts::exec Exec>
io_expected basic_tcp_connection<Exec>::write_all
(std::span<const const_buffer> buffers) noexcept
{
	error_code error {};
	auto size = asio::write(m_socket, buffers, error);
	if( error )
		return io_unexpected(error);
	return size;
}

template <core_concepts::exec Exec>
awaitable<io_expected>
basic_tcp_connection<Exec>::co_read_some(mutable_buffer buffer) noexcept
{
	error_code error {};
	auto size = co_await m_socket.async_read_some (
		buffer, asio::redirect_error(use_awaitable, error)
	);
	if( error )
		co_return io_unexpected(error);
	co_return size;
}

template <core_concepts::exec Exec>
awaitable<io_expected>
basic_tcp_connection<Exec>::co_write_all(const_buffer buffer) noexcept
{
	error_code error {};
	auto size = co_await asio::async_write(m_socket, buffer,
		asio::redirect_error(use_awaitable, error)
	);
	if( error )
		co_return io_unexpected(error);
	co_return size;
}

template <core_concepts::exec Exec>
awaitable<io_expected>
basic_tcp_connection<Exec>::co_write_all(std::span<const const_buffer> buffers) noexcept
{
	error_code error {};
	auto size = co_await asio::async_write(m_socket, buffers,
		asio::redirect_error(use_awaitable, error)
	);
	if( error )
		co_return io_unexpected(error);
	co_return size;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_UTILS_DETAIL_TCP_CONNECTION_H
