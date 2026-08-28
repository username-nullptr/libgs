
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

#ifndef LIBGS_HTTP_UTILS_DETAIL_CONNECTION_H
#define LIBGS_HTTP_UTILS_DETAIL_CONNECTION_H

#include <libgs/http/utils/detail/async_expected.h>

namespace libgs::http { namespace detail
{

template <typename Socket>
[[nodiscard]] sys_expected<connection_probe_state>
probe_tcp_socket(Socket &socket) noexcept
{
	if( not socket.is_open() )
		return connection_probe_state::peer_closed;

	error_code error {};
	const bool was_non_blocking = socket.non_blocking();
	socket.non_blocking(true, error);
	if( error )
		return sys_unexpected(error);

	char byte = 0;
	auto size = socket.receive(asio::buffer(&byte, 1),
		asio::socket_base::message_peek, error
	);

	error_code restore_error {};
	socket.non_blocking(was_non_blocking, restore_error);
	if( restore_error )
		return sys_unexpected(restore_error);

	if( not error )
	{
		return size == 0 ? connection_probe_state::peer_closed
			: connection_probe_state::data_pending;
	}
	if( error == errc::would_block or error == errc::try_again )
		return connection_probe_state::no_event;

	if( error == errc::eof or error == errc::connection_reset or
		error == errc::connection_aborted or error == errc::not_connected or
		error == errc::bad_descriptor )
	{
		return connection_probe_state::peer_closed;
	}
	return sys_unexpected(error);
}

inline endpoint to_endpoint(const asio::ip::tcp::endpoint &value) noexcept
{
	return {value.address(), value.port()};
}

template <typename Socket>
[[nodiscard]] sys_expected<> set_tcp_socket_options
(Socket &socket, const tcp_socket_options &options) noexcept
{
	error_code error {};
	auto set = [&socket, &error](const auto &option) {
		socket.set_option(option, error);
		return not error;
	};

	if( options.no_delay and not set(asio::ip::tcp::no_delay(*options.no_delay)) )
		return sys_unexpected(error);
	if( options.keep_alive and not set(asio::socket_base::keep_alive(*options.keep_alive)) )
		return sys_unexpected(error);

	if( options.send_buffer_size )
	{
		if( *options.send_buffer_size > static_cast<size_t>(std::numeric_limits<int>::max()) )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		if( not set(asio::socket_base::send_buffer_size(
			static_cast<int>(*options.send_buffer_size))) )
			return sys_unexpected(error);
	}
	if( options.receive_buffer_size )
	{
		if( *options.receive_buffer_size > static_cast<size_t>(std::numeric_limits<int>::max()) )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		if( not set(asio::socket_base::receive_buffer_size(
			static_cast<int>(*options.receive_buffer_size))) )
			return sys_unexpected(error);
	}
	if( options.linger and not set(*options.linger) )
		return sys_unexpected(error);
	return make_sys_expected();
}

template <typename Socket>
[[nodiscard]] sys_expected<tcp_socket_state>
get_tcp_socket_options(const Socket &socket) noexcept
{
	tcp_socket_state state {};
	error_code error {};

	asio::ip::tcp::no_delay no_delay {};
	socket.get_option(no_delay, error);
	if( error )
		return sys_unexpected(error);
	state.no_delay = no_delay.value();

	asio::socket_base::keep_alive keep_alive {};
	socket.get_option(keep_alive, error);
	if( error )
		return sys_unexpected(error);
	state.keep_alive = keep_alive.value();

	asio::socket_base::send_buffer_size send_size {};
	socket.get_option(send_size, error);
	if( error )
		return sys_unexpected(error);
	state.send_buffer_size = static_cast<size_t>(send_size.value());

	asio::socket_base::receive_buffer_size receive_size {};
	socket.get_option(receive_size, error);
	if( error )
		return sys_unexpected(error);
	state.receive_buffer_size = static_cast<size_t>(receive_size.value());

	socket.get_option(state.linger, error);
	if( error )
		return sys_unexpected(error);
	return state;
}

} //namespace detail

template <core_concepts::exec Exec>
basic_connection<Exec>::~basic_connection() = default;

template <core_concepts::exec Exec>
template <typename Token>
auto basic_connection<Exec>::read(const mutable_buffer &buf, Token &&token)
	noexcept requires task_token_v<Token,size_t>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		auto result = read_some(buf);
		token = result ? error_code{} : result.error();
		return result ? *result : size_t{};
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return read_some(buf);
	else
	{
		return detail::initiate_expected<size_t>(get_executor(),
		[this, buf]() mutable -> awaitable<io_expected> {
			co_return co_await co_read_some(buf);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_connection<Exec>::write(const const_buffer &body, Token &&token) noexcept
{
	if constexpr( is_error_code_token_v<Token> )
	{
		auto result = write_all(body);
		token = result ? error_code{} : result.error();
		return result ? *result : size_t{};
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return write_all(body);
	else
	{
		return detail::initiate_expected<size_t>(get_executor(),
		[this, body]() mutable -> awaitable<io_expected> {
			co_return co_await co_write_all(body);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_connection<Exec>::write
(std::span<const const_buffer> buffers, Token &&token) noexcept
{
	if constexpr( is_error_code_token_v<Token> )
	{
		auto result = write_all(buffers);
		token = result ? error_code{} : result.error();
		return result ? *result : size_t{};
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return write_all(buffers);
	else
	{
		std::vector<const_buffer> sequence(buffers.begin(), buffers.end());
		return detail::initiate_expected<size_t>(get_executor(),
		[this, sequence = std::move(sequence)]() mutable -> awaitable<io_expected> {
			co_return co_await co_write_all(sequence);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
io_expected basic_connection<Exec>::write_all
(std::span<const const_buffer> buffers) noexcept
{
	size_t sum = 0;
	for( const auto &buffer : buffers )
	{
		auto result = write_all(buffer);
		if( not result )
			return result;
		sum += *result;
	}
	return sum;
}

template <core_concepts::exec Exec>
awaitable<io_expected> basic_connection<Exec>::co_write_all
(std::span<const const_buffer> buffers) noexcept
{
	size_t sum = 0;
	for( const auto &buffer : buffers )
	{
		auto result = co_await co_write_all(buffer);
		if( not result )
			co_return result;
		sum += *result;
	}
	co_return sum;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_UTILS_DETAIL_CONNECTION_H
