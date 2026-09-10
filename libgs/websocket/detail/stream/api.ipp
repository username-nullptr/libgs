// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_API_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_API_IPP

namespace libgs::websocket
{

template <core_concepts::exec Exec>
basic_stream<Exec>::basic_stream()
	requires core_concepts::match_sched<io_executor_t, executor_t>
	: basic_stream(executor_t(libgs::get_executor()))
{
}

template <core_concepts::exec Exec>
basic_stream<Exec>::basic_stream(config_t config)
	requires core_concepts::match_sched<io_executor_t, executor_t>
	: basic_stream(executor_t(libgs::get_executor()), config)
{
}

template <core_concepts::exec Exec>
basic_stream<Exec>::basic_stream(
	core_concepts::match_sched<executor_t> auto &&exec, config_t config)
	: m_impl(std::make_shared<impl>(
		  get_executor_helper(std::forward<decltype(exec)>(exec)), config))
{
}

template <core_concepts::exec Exec>
basic_stream<Exec>::basic_stream(basic_stream &&other) noexcept
	: m_impl(std::move(other.m_impl))
{
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::operator=(basic_stream &&other) noexcept
{
	if(this == &other)
		return *this;
	if(m_impl)
	{
		error_code error;
		m_impl->shutdown(error);
	}
	m_impl = std::move(other.m_impl);
	return *this;
}

template <core_concepts::exec Exec>
basic_stream<Exec>::~basic_stream()
{
	if(m_impl)
	{
		error_code error;
		m_impl->shutdown(error);
	}
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::adopt(
	connection_ptr connection, adopt_options_t options)
{
	error_code error;
	m_impl->adopt(std::move(connection), std::move(options), error);
	if(error)
		system_error::loc_throw(error, "libgs::websocket::basic_stream::adopt");
	return *this;
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::adopt(
	connection_ptr connection, adopt_options_t options,
	error_code &error) noexcept
{
	m_impl->adopt(std::move(connection), std::move(options), error);
	return *this;
}

template <core_concepts::exec Exec>
template <typename Buffer, typename Token>
auto basic_stream<Exec>::read(Token &&token)
	requires message_buffer_v<Buffer> and
	task_token_v<Token, basic_message<std::remove_cvref_t<Buffer>>>
{
	using buffer_t = std::remove_cvref_t<Buffer>;
	using result_t = basic_message<buffer_t>;
	if constexpr(is_error_code_token_v<Token>)
	{
		auto value = m_impl->read(token);
		if(token)
			return result_t{};
		return impl::template convert_message<buffer_t>(std::move(value), token);
	}
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto value = m_impl->read(error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::basic_stream::read");
		auto result = impl::template convert_message<buffer_t>(
			std::move(value), error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::basic_stream::read");
		return result;
	}
	else
	{
		return initiate_io<result_t>(get_executor(), [self = m_impl]<typename T0>(T0 &&completion_token) mutable
			{
				auto slot = asio::get_associated_cancellation_slot(completion_token);
				auto completion_exec = asio::get_associated_executor(
				completion_token, self->m_exec);
				auto allocator = asio::get_associated_allocator(completion_token);
				auto bridge = asio::bind_allocator(allocator,
				asio::bind_executor(completion_exec,
				asio::bind_cancellation_slot(slot,
				[handler = std::forward<T0>(completion_token)](
				error_code error, message value) mutable
				{
					if( error )
					{
						std::move(handler)(error, result_t {});
						return;
					}
					auto result = impl::template convert_message<buffer_t>(
					std::move(value), error);
					std::move(handler)(error, std::move(result));
				})));
				self->async_read_message(std::move(bridge)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <message_type Type, typename Token>
auto basic_stream<Exec>::write(const const_buffer &body, Token &&token)
	requires completion_token_v<Token, size_t>
{
	static_assert(Type == message_type::text or Type == message_type::binary);
	return write(Type, body, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::write(message_type type, const const_buffer &body,
	Token &&token)
	requires completion_token_v<Token, size_t>
{
	return write(type, std::span<const const_buffer>(&body, 1),
		std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::write(message_type type,
	std::span<const const_buffer> body, Token &&token)
	requires completion_token_v<Token, size_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->write(type, body, token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto transferred = m_impl->write(type, body, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::basic_stream::write");
		return transferred;
	}
	else
	{
		auto buffers = std::vector<const_buffer>(body.begin(), body.end());
		std::shared_ptr<std::vector<std::byte>> payload_owner;
		error_code buffer_error {};

		// A detached operation has no caller-managed completion boundary, so it
		// takes ownership before initiation returns. Every other async token keeps
		// Asio's borrowed-buffer contract and performs no lifetime copy here.
		if constexpr( is_detached_v<token_unbound_t<Token>> )
		{
			try
			{
				size_t size = 0;
				for(const auto &input : buffers)
				{
					if( input.size() != 0 and input.data() == nullptr )
					{
						buffer_error = make_error_code(std::errc::invalid_argument);
						break;
					}
					if( input.size() > std::numeric_limits<size_t>::max() - size )
					{
						buffer_error = make_error_code(std::errc::value_too_large);
						break;
					}
					size += input.size();
				}
				if( not buffer_error )
				{
					payload_owner =
						std::make_shared<std::vector<std::byte>>(size);
					size_t offset = 0;
					for(const auto &input : buffers)
					{
						if( input.size() == 0 )
							continue;
						std::memcpy(payload_owner->data() + offset,
							input.data(), input.size());
						offset += input.size();
					}
					buffers.assign(1, const_buffer(
						payload_owner->data(), payload_owner->size()
					));
				}
				else
					buffers.clear();
			}
			catch(const std::bad_alloc &)
			{
				buffer_error = make_error_code(std::errc::not_enough_memory);
				buffers.clear();
			}
			catch(...)
			{
				buffer_error = make_error_code(std::errc::io_error);
				buffers.clear();
			}
		}
		return initiate_io<size_t>(get_executor(),
			[self = m_impl, type, buffers = std::move(buffers),
			 payload_owner = std::move(payload_owner), buffer_error]
			<typename T0>(T0 &&completion_token) mutable
			{
				if( buffer_error )
				{
					asio::post(self->m_exec,
						[handler = std::forward<T0>(completion_token), buffer_error]() mutable {
							std::move(handler)(buffer_error, 0);
						});
					return ;
				}
				self->async_write_message(type, buffers, std::move(payload_owner),
					std::forward<T0>(completion_token));
			}, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::write_text(std::string_view text, Token &&token)
	requires completion_token_v<Token, size_t>
{
	return write(message_type::text, const_buffer(text.data(), text.size()),
		std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::write_binary(const const_buffer &body, Token &&token)
	requires completion_token_v<Token, size_t>
{
	return write(message_type::binary, body, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::wait_written(Token &&token)
	requires task_token_v<Token>
{
	if constexpr(is_error_code_token_v<Token>)
		m_impl->wait_written(token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		m_impl->wait_written(error);
		if(error)
			system_error::loc_throw(error,
				"libgs::websocket::basic_stream::wait_written");
	}
	else
	{
		return initiate_io_void(get_executor(), [self = m_impl]<typename T0>(T0 &&completion_token) mutable
			{ self->async_wait_written(std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::wait_ctrl(Token &&token)
	requires task_token_v<Token, control_event_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->wait_control(token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto event = m_impl->wait_control(error);
		if(error)
			system_error::loc_throw(error,
				"libgs::websocket::basic_stream::wait_ctrl");
		return event;
	}
	else
	{
		return initiate_io<control_event_t>(get_executor(), [self = m_impl]<typename T0>(T0 &&completion_token) mutable
			{ self->async_wait_control(std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::ping(Token &&token)
	requires task_token_v<Token, size_t>
{
	return ping(const_buffer{}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::ping(const const_buffer &payload, Token &&token)
	requires task_token_v<Token, size_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->write_control(opcode::ping, payload, token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto transferred = m_impl->write_control(opcode::ping, payload, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::basic_stream::ping");
		return transferred;
	}
	else
	{
		return initiate_io<size_t>(get_executor(), [self = m_impl, payload]<typename T0>(T0 &&completion_token) mutable
			{ self->async_write_control(opcode::ping, payload,
				  std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::pong(Token &&token)
	requires task_token_v<Token, size_t>
{
	return pong(const_buffer{}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::pong(const const_buffer &payload, Token &&token)
	requires task_token_v<Token, size_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->write_control(opcode::pong, payload, token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto transferred = m_impl->write_control(opcode::pong, payload, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::basic_stream::pong");
		return transferred;
	}
	else
	{
		return initiate_io<size_t>(get_executor(), [self = m_impl, payload]<typename T0>(T0 &&completion_token) mutable
			{ self->async_write_control(opcode::pong, payload,
				  std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::close(Token &&token)
	requires completion_token_v<Token, close_info_t>
{
	return close(close_frame_t{}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::close(close_frame_t frame, Token &&token)
	requires completion_token_v<Token, close_info_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->close(std::move(frame), token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto result = m_impl->close(std::move(frame), error);
		if(error)
			system_error::loc_throw(error,
				"libgs::websocket::basic_stream::close");
		return result;
	}
	else
	{
		return initiate_io<close_info_t>(get_executor(), [self = m_impl, frame = std::move(frame)]<typename T0>(T0 &&completion_token) mutable
			{ self->async_close(std::move(frame),
				  std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_stream<Exec>::wait_closed(Token &&token)
	requires task_token_v<Token, close_info_t>
{
	if constexpr(is_error_code_token_v<Token>)
		return m_impl->wait_closed(token);
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		error_code error;
		auto result = m_impl->wait_closed(error);
		if(error)
			system_error::loc_throw(error,
				"libgs::websocket::basic_stream::wait_closed");
		return result;
	}
	else
	{
		return initiate_io<close_info_t>(get_executor(), [self = m_impl]<typename T0>(T0 &&completion_token) mutable
			{ self->async_wait_closed(std::forward<T0>(completion_token)); }, std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
connection_state basic_stream<Exec>::state() const noexcept
{
	return m_impl ? m_impl->m_state : connection_state::closed;
}

template <core_concepts::exec Exec>
bool basic_stream<Exec>::is_open() const noexcept
{
	return state() == connection_state::open;
}

template <core_concepts::exec Exec>
bool basic_stream<Exec>::is_closing() const noexcept
{
	return state() == connection_state::closing;
}

template <core_concepts::exec Exec>
role basic_stream<Exec>::stream_role() const noexcept
{
	return m_impl ? m_impl->m_role : role::client;
}

template <core_concepts::exec Exec>
std::string basic_stream<Exec>::negotiated_subprotocol() const
{
	return m_impl ? m_impl->m_subprotocol : std::string{};
}

template <core_concepts::exec Exec>
std::vector<extension> basic_stream<Exec>::negotiated_extensions() const
{
	return m_impl ? m_impl->m_extensions : std::vector<extension>{};
}

template <core_concepts::exec Exec>
basic_stream<Exec>::config_t basic_stream<Exec>::config() const noexcept
{
	return m_impl ? m_impl->m_config : config_t{};
}

template <core_concepts::exec Exec>
std::optional<typename basic_stream<Exec>::close_info_t>
basic_stream<Exec>::peer_close() const
{
	return m_impl ? m_impl->m_peer_close : std::nullopt;
}

template <core_concepts::exec Exec>
http::endpoint basic_stream<Exec>::remote_endpoint() const noexcept
{
	return m_impl and m_impl->m_connection ? m_impl->m_connection->remote_endpoint() : http::endpoint{};
}

template <core_concepts::exec Exec>
http::endpoint basic_stream<Exec>::local_endpoint() const noexcept
{
	return m_impl and m_impl->m_connection ? m_impl->m_connection->local_endpoint() : http::endpoint{};
}

template <core_concepts::exec Exec>
basic_stream<Exec>::executor_t basic_stream<Exec>::get_executor() const noexcept
{
	return m_impl->m_exec;
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::cancel()
{
	error_code error;
	cancel(error);
	if(error)
		system_error::loc_throw(error, "libgs::websocket::basic_stream::cancel");
	return *this;
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::cancel(error_code &error) noexcept
{
	m_impl->cancel(error);
	return *this;
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::shutdown()
{
	error_code error;
	shutdown(error);
	if(error)
		system_error::loc_throw(error, "libgs::websocket::basic_stream::shutdown");
	return *this;
}

template <core_concepts::exec Exec>
basic_stream<Exec> &basic_stream<Exec>::shutdown(error_code &error) noexcept
{
	m_impl->shutdown(error);
	return *this;
}

} // namespace libgs::websocket

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_API_IPP
