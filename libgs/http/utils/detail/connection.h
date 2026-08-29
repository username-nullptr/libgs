
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

class const_buffer_sequence
{
	static constexpr size_t inline_capacity = 4;

public:
	using value_type = const_buffer;
	using const_iterator = const value_type*;

	explicit const_buffer_sequence(std::span<const const_buffer> buffers)
	{
		m_size = buffers.size();
		if( m_size <= inline_capacity )
			std::ranges::copy(buffers, m_inline.begin());
		else
			m_dynamic.assign(buffers.begin(), buffers.end());
	}

	[[nodiscard]] std::span<const const_buffer> buffers() const noexcept
	{
		if( m_size <= inline_capacity )
			return {m_inline.data(), m_size};
		return m_dynamic;
	}

	[[nodiscard]] const_iterator begin() const noexcept {
		return buffers().data();
	}

	[[nodiscard]] const_iterator end() const noexcept {
		auto sequence = buffers();
		return sequence.data() + sequence.size();
	}

private:
	std::array<const_buffer,inline_capacity> m_inline {};
	std::vector<const_buffer> m_dynamic {};
	size_t m_size = 0;
};

template <typename Token, typename Initiation>
[[nodiscard]] auto initiate_connection_io(Initiation initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t ntoken(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,size_t)>(
		std::move(initiation), ntoken
	);
}

template <bool OwnsBuffer = false, core_concepts::exec Exec, typename Token, typename Initiation>
[[nodiscard]] auto initiate_connection_io
(const Exec &exec, Initiation initiation, Token &&token, std::shared_ptr<void> owner = {})
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		return initiate_expected<size_t>(exec,
		[initiation = std::move(initiation), owner = std::move(owner)]()
		mutable -> awaitable<io_expected>
		{
			error_code error {};
			size_t size = 0;
			if constexpr( OwnsBuffer )
			{
				size = co_await initiate_connection_io(
					std::move(initiation),
					asio::consign(
						asio::redirect_error(use_awaitable, error), owner
					)
				);
			}
			else
			{
				size = co_await initiate_connection_io(
					std::move(initiation),
					asio::redirect_error(use_awaitable, error)
				);
			}
			if( error )
				co_return io_unexpected(error);
			co_return size;
		},
		std::forward<Token>(token));
	}
	else if constexpr( OwnsBuffer )
	{
		auto owned_token = asio::consign(
			std::forward<Token>(token), std::move(owner)
		);
		return initiate_connection_io(
			std::move(initiation), std::move(owned_token)
		);
	}
	else
	{
		return initiate_connection_io(
			std::move(initiation), std::forward<Token>(token)
		);
	}
}

inline std::shared_ptr<std::string> copy_buffer(const const_buffer &buffer)
{
	auto result = std::make_shared<std::string>();
	if( buffer.size() > 0 )
	{
		result->assign(
			static_cast<const char*>(buffer.data()), buffer.size()
		);
	}
	return result;
}

inline std::shared_ptr<std::string>
copy_buffers(std::span<const const_buffer> buffers)
{
	auto result = std::make_shared<std::string>();
	size_t size = 0;
	for( const auto &buffer : buffers )
	{
		if( buffer.size() > result->max_size() - size )
			length_error::loc_throw("libgs::http::basic_connection::write");
		size += buffer.size();
	}
	result->reserve(size);
	for( const auto &buffer : buffers )
	{
		if( buffer.size() > 0 )
		{
			result->append(
				static_cast<const char*>(buffer.data()), buffer.size()
			);
		}
	}
	return result;
}

template <typename Socket>
[[nodiscard]] sys_expected<connection_probe_state> probe_tcp_socket(Socket &socket) noexcept
{
	if( not socket.is_open() )
		return connection_probe_state::peer_closed;

	error_code error {};
	bool was_non_blocking = socket.non_blocking();

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
		return connection_probe_state::peer_closed;

	return sys_unexpected(error);
}

inline endpoint to_endpoint(const asio::ip::tcp::endpoint &value) noexcept
{
	return { .address = value.address(), .port = value.port() };
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

		if( not set(asio::socket_base::send_buffer_size(static_cast<int>(*options.send_buffer_size))) )
			return sys_unexpected(error);
	}
	if( options.receive_buffer_size )
	{
		if( *options.receive_buffer_size > static_cast<size_t>(std::numeric_limits<int>::max()) )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		if( not set(asio::socket_base::receive_buffer_size(static_cast<int>(*options.receive_buffer_size))) )
			return sys_unexpected(error);
	}
	if( options.linger and not set(*options.linger) )
		return sys_unexpected(error);
	return make_sys_expected();
}

template <typename Socket>
[[nodiscard]] sys_expected<tcp_socket_state> get_tcp_socket_options(const Socket &socket) noexcept
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
		return detail::initiate_connection_io(get_executor(),
		[this, buf](auto handler) mutable {
			co_read_some(buf, io_handler_t(std::move(handler)));
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
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_detached_v<token_unbound_t<token_t>> )
		{
			auto owner = detail::copy_buffer(body);
			return detail::initiate_connection_io<true>(get_executor(),
			[this, owner](auto handler) mutable
			{
				co_write_all (
					const_buffer(*owner), io_handler_t(std::move(handler))
				);
			},
			std::forward<Token>(token), owner);
		}
		else
		{
			return detail::initiate_connection_io(get_executor(),
			[this, body](auto handler) mutable {
				co_write_all(body, io_handler_t(std::move(handler)));
			},
			std::forward<Token>(token));
		}
	}
}

template <core_concepts::exec Exec>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_connection<Exec>::write(std::span<const const_buffer> buffers, Token &&token) noexcept
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
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_detached_v<token_unbound_t<token_t>> )
		{
			auto owner = detail::copy_buffers(buffers);
			return detail::initiate_connection_io<true>(get_executor(),
			[this, owner](auto handler) mutable
			{
				co_write_all (
					const_buffer(*owner), io_handler_t(std::move(handler))
				);
			},
			std::forward<Token>(token), owner);
		}
		else
		{
			detail::const_buffer_sequence sequence(buffers);
			return detail::initiate_connection_io(get_executor(),
			[this, sequence = std::move(sequence)](auto handler) mutable
			{
				co_write_all (
					sequence.buffers(), io_handler_t(std::move(handler))
				);
			},
			std::forward<Token>(token));
		}
	}
}

template <core_concepts::exec Exec>
io_expected basic_connection<Exec>::write_all(std::span<const const_buffer> buffers) noexcept
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
void basic_connection<Exec>::co_write_all
(std::span<const const_buffer> buffers, io_handler_t handler) noexcept
{
	auto exec = get_executor();
	auto slot = asio::get_associated_cancellation_slot(handler);

	auto completion_exec = asio::get_associated_executor(handler, exec);
	detail::const_buffer_sequence sequence(buffers);

	asio::co_spawn(exec,
	[this, sequence = std::move(sequence)]() mutable -> awaitable<io_expected>
	{
		size_t sum = 0;
		for( const auto &buffer : sequence.buffers() )
		{
			error_code error {};
			auto size = co_await detail::initiate_connection_io (
			[this, buffer](auto next_handler) mutable
			{
				co_write_all (
					buffer, io_handler_t(std::move(next_handler))
				);
			},
			asio::redirect_error(use_awaitable, error));
			if( error )
				co_return io_unexpected(error);
			sum += size;
		}
		co_return sum;
	},
	asio::bind_executor(completion_exec, asio::bind_cancellation_slot(slot,
	[handler = std::move(handler)](const std::exception_ptr &exception, io_expected result) mutable
	{
		if( exception )
		{
			try {
				std::rethrow_exception(exception);
			}
			catch(const std::system_error &ex) {
				std::move(handler)(ex.code(), 0);
			}
			catch(const std::bad_alloc&)
			{
				std::move(handler) (
					make_error_code(std::errc::not_enough_memory), 0
				);
			}
			catch(...)
			{
				std::move(handler) (
					make_error_code(std::errc::io_error), 0
				);
			}
			return ;
		}
		if( not result )
			std::move(handler)(result.error(), 0);
		else
			std::move(handler)(error_code{}, *result);
	})));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_UTILS_DETAIL_CONNECTION_H
