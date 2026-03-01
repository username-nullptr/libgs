
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
		m_socket(std::move(other.m_socket)) {
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
  		if( not m_destructor )
  			return ;
		dispatch(m_opt_helper.get_executor(),
		[destructor = std::move(m_destructor), socket = std::move(m_socket)]() mutable
		{
			opt_helper_t(socket).cancel();
			destructor(std::move(socket));
		});
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

template <concepts::stream Stream>
auto basic_connection<Stream>::set_send_file_option() noexcept
{
	using protocol_t = opt_helper_t::protocol_t;
	constexpr size_t net_buf_size = 8 * 1024 * 1024;

	auto &socket = opt_helper();
	error_code error;

	if constexpr( std::is_same_v<protocol_t, asio::ip::tcp> )
	{
		using tuple_t = std::tuple <
			asio::socket_base::send_buffer_size,
			asio::ip::tcp::no_delay,
			asio::socket_base::linger
		>;
		sys_expected<tuple_t> result {};

		asio::socket_base::send_buffer_size send_buffer_size;
		socket.get_option(send_buffer_size, error);
		if( error )
			return result.despair(error);

		asio::ip::tcp::no_delay no_delay; // Nagle
		socket.get_option(no_delay, error);
		if( error )
			return result.despair(error);

		asio::socket_base::linger linger;
		socket.get_option(linger, error);
		if( error )
			return result.despair(error);

		result.emplace(std::move(send_buffer_size),
			std::move(no_delay), std::move(linger)
		);
		send_buffer_size = net_buf_size;
		socket.set_option(send_buffer_size, error);
		if( error )
			return result.despair(error);

		no_delay = true;
		socket.set_option(no_delay, error);
		if( error )
			return result.despair(error);

		linger.enabled(false);
		linger.timeout(0);
		socket.set_option(linger, error);
		if( error )
			return result.despair(error);
		return result;
	}
	else
	{
		using tuple_t = std::tuple <
			asio::socket_base::send_buffer_size,
			asio::socket_base::linger
		>;
		sys_expected<tuple_t> result {};

		asio::socket_base::send_buffer_size send_buffer_size;
		socket.get_option(send_buffer_size, error);
		if( error )
			return result.despair(error);

		asio::socket_base::linger linger;
		socket.get_option(linger, error);
		if( error )
			return result.despair(error);

		result.emplace (
			std::move(send_buffer_size), std::move(linger)
		);
		send_buffer_size = net_buf_size;
		socket.set_option(send_buffer_size, error);
		if( error )
			return result.despair(error);

		linger.enabled(false);
		linger.timeout(0);
		socket.set_option(linger, error);
		if( error )
			return result.despair(error);
		return result;
	}
}

template <concepts::stream Stream>
auto basic_connection<Stream>::set_receive_file_option() noexcept
{
	constexpr size_t net_buf_size = 8 * 1024 * 1024;
	auto &socket = opt_helper();
	error_code error;

	using tuple_t = std::tuple <
		asio::socket_base::receive_buffer_size,
		asio::socket_base::linger
	>;
	sys_expected<tuple_t> result {};

	asio::socket_base::receive_buffer_size recv_buffer_size;
	socket.get_option(recv_buffer_size, error);
	if( error )
		return result.despair(error);

	asio::socket_base::linger linger;
	socket.get_option(linger, error);
	if( error )
		return result.despair(error);

	recv_buffer_size = net_buf_size;
	socket.set_option(recv_buffer_size, error);
	if( error )
		return result.despair(error);

	linger.enabled(false);
	linger.timeout(0);
	socket.set_option(linger, error);
	if( error )
		return result.despair(error);
	return result;
}

template <concepts::stream Stream>
auto basic_connection<Stream>::unset_transfer_file_option(const auto &before) noexcept
{
	auto &socket = opt_helper();
	sys_expected<> result;
	error_code error;

	std::apply([&]<typename...Args>(Args&&...args)
	{
		([&]<typename Arg>(Arg &&arg)
		{
			socket.set_option(std::forward<Arg>(arg), error);
			if( error )
			{
				result.despair(error);
				return false;
			}
			return true;
		}
		(std::forward<Args>(args)) && ...);
	},
	before);
	return result;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_UTILS_DETAIL_CONNECTION_H