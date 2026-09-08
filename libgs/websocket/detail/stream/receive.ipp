// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_IPP

// Frame consumption and message receive operations.

template <core_concepts::exec Exec>
mutable_buffer stream_impl<Exec>::available_read_data() noexcept
{
	if(m_pending_offset < m_pending_data.size())
	{
		return mutable_buffer(m_pending_data.data() + m_pending_offset,
			m_pending_data.size() - m_pending_offset);
	}
	if(m_read_offset < m_read_size)
	{
		return mutable_buffer(m_read_buffer->data() + m_read_offset,
			m_read_size - m_read_offset);
	}
	return {};
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::consume_read_data(size_t size) noexcept
{
	if(m_pending_offset < m_pending_data.size())
	{
		m_pending_offset += size;
		if(m_pending_offset == m_pending_data.size())
		{
			m_pending_data.clear();
			m_pending_offset = 0;
		}
		return;
	}
	m_read_offset += size;
	if(m_read_offset == m_read_size)
	{
		m_read_offset = 0;
		m_read_size = 0;
	}
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::consume_frame_data() noexcept
	-> sys_expected<std::optional<received_event>>
{
	auto input = available_read_data();
	if(input.size() == 0)
		return std::optional<received_event>{};

	auto parsed = m_parser->parse(input);
	if(not parsed)
		return sys_unexpected(parsed.error());
	if(parsed->consumed == 0)
		return sys_unexpected(make_error_code(std::errc::io_error));

	const auto &header = m_parser->header();
	if(parsed->header_ready)
	{
		if(header.op == opcode::text or header.op == opcode::binary)
		{
			m_message_type = header.op == opcode::text ? message_type::text : message_type::binary;
			m_message_body.clear();
		}
		else if(is_control_opcode(header.op))
			m_control_body.clear();

		if(is_data_opcode(header.op) or header.op == opcode::continuation)
		{
			if(header.payload_size > std::numeric_limits<size_t>::max() or
				static_cast<size_t>(header.payload_size) >
					m_message_body.max_size() - m_message_body.size())
				return sys_unexpected(make_error_code(errc::message_too_big));

			if(m_config.max_message_size != 0 and
				static_cast<size_t>(header.payload_size) >
					m_config.max_message_size -
						std::min(m_message_body.size(), m_config.max_message_size))
				return sys_unexpected(make_error_code(errc::message_too_big));
		}
	}

	if(parsed->payload.size() != 0)
	{
		if(header.mask)
			apply_mask(parsed->payload, *header.mask, parsed->payload_offset);
		try
		{
			const auto *begin = static_cast<const std::byte *>(parsed->payload.data());
			auto &destination = is_control_opcode(header.op) ? m_control_body : m_message_body;
			destination.insert(destination.end(), begin,
				begin + parsed->payload.size());
		}
		catch(const std::bad_alloc &)
		{
			return sys_unexpected(make_error_code(std::errc::not_enough_memory));
		}
		catch(...)
		{
			return sys_unexpected(make_error_code(std::errc::io_error));
		}
	}

	consume_read_data(parsed->consumed);
	if(not parsed->frame_finished)
		return std::optional<received_event>{};

	received_event event{.op = header.op};
	if(is_data_opcode(header.op) or header.op == opcode::continuation)
	{
		if(header.fin)
		{
			if(not m_message_type)
				return sys_unexpected(make_error_code(
					protocol_errc::unexpected_continuation));
			if(*m_message_type == message_type::text)
			{
				const auto text = m_message_body.empty() ? std::string_view{} : std::string_view(reinterpret_cast<const char *>(m_message_body.data()), m_message_body.size());
				if(not detail::is_valid_utf8(text))
					return sys_unexpected(make_error_code(protocol_errc::invalid_utf8));
			}
			event.data = message{
				.type = *m_message_type,
				.body = std::move(m_message_body),
			};
			m_message_type.reset();
			m_message_body.clear();
		}
	}
	else
		event.control = std::move(m_control_body);
	return std::optional<received_event>(std::move(event));
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::close_transport(error_code &error) noexcept
{
	if(not m_connection or m_transport_closed)
	{
		error.clear();
		return;
	}
	auto result = m_connection->close();
	m_transport_closed = true;
	error = result ? error_code{} : result.error();
}

template <core_concepts::exec Exec>
message stream_impl<Exec>::finish_read_error(error_code &error) noexcept
{
	if(error == asio::error::operation_aborted)
		return {};

	if(error == asio::error::eof)
	{
		error_code close_error;
		close_transport(close_error);
		if(close_error)
		{
			fail(close_error);
			error = close_error;
		}
		else
		{
			m_close_result = retained_close_info(false);
			m_state = connection_state::closed;
			stop_control_observer(asio::error::eof);
			complete_close_waiters(asio::error::eof);
		}
		return {};
	}

	fail(error);
	return {};
}

template <core_concepts::exec Exec>
message stream_impl<Exec>::read(error_code &error) noexcept
{
	error.clear();
	if(m_state == connection_state::idle)
	{
		error = make_error_code(errc::not_open);
		return {};
	}
	if(m_state == connection_state::closing)
	{
		error = make_error_code(errc::closing);
		return {};
	}
	if(m_state == connection_state::closed)
	{
		error = asio::error::eof;
		return {};
	}
	if(m_state == connection_state::failed)
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return {};
	}
	if(m_read_active)
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}

	m_read_active = true;
	struct read_guard
	{
		bool &active;
		~read_guard()
		{
			active = false;
		}
	} guard{m_read_active};

	for(;;)
	{
		while(available_read_data().size() != 0)
		{
			auto event = consume_frame_data();
			if(not event)
			{
				error = event.error();
				begin_protocol_failure(error, true);
				return {};
			}
			if(not*event)
				continue;

			auto &value = **event;
			if(value.data)
				return std::move(*value.data);

			if(value.op == opcode::ping or value.op == opcode::pong)
			{
				if(value.op == opcode::ping and m_config.automatic_pong)
				{
					if(send_engine_busy())
					{
						auto retained = queue_automatic_pong(value.control);
						if(not retained)
						{
							error = retained.error();
							fail(error);
							return {};
						}
						remember_control(value.op, std::move(value.control));
						continue;
					}
					auto frame = prepare_control_frame(opcode::pong,
						const_buffer(value.control.data(), value.control.size()));
					if(not frame)
					{
						error = frame.error();
						fail(error);
						return {};
					}
					ignore_unused(write_prepared(*frame, error));
					if(error)
					{
						fail(error);
						return {};
					}
				}
				remember_control(value.op, std::move(value.control));
				continue;
			}

			if(value.op == opcode::close)
			{
				auto remembered = remember_peer_close(value.control);
				if(not remembered)
				{
					error = remembered.error();
					begin_protocol_failure(error, true);
					return {};
				}
				if(send_engine_busy())
				{
					start_close_deadline();
					if(m_state == connection_state::failed)
					{
						error = m_error;
						return {};
					}
					auto retained = retain_protocol_payload(
						m_pending_close_response, value.control);
					if(not retained)
					{
						error = retained.error();
						fail(error);
						return {};
					}
					m_pending_auto_pong.reset();
					fail_queued_writes(make_error_code(std::errc::broken_pipe));
					schedule_send();
					error = asio::error::eof;
					return {};
				}
				auto frame = prepare_control_frame(opcode::close,
					const_buffer(value.control.data(), value.control.size()));
				if(not frame)
				{
					error = frame.error();
					fail(error);
					return {};
				}
				ignore_unused(write_prepared(*frame, error));
				if(error)
				{
					fail(error);
					return {};
				}
				finish_close({}, true);
				error = asio::error::eof;
				return {};
			}
		}

		if(m_read_error)
		{
			error = std::exchange(m_read_error, {});
			return finish_read_error(error);
		}

		error_code read_error;
		const auto size = m_connection->read(mutable_buffer(
												 m_read_buffer->data(), m_read_buffer->size()),
			read_error);
		if(size > m_read_buffer->size())
		{
			error = make_error_code(std::errc::io_error);
			fail(error);
			return {};
		}
		m_read_size = size;
		m_read_offset = 0;
		m_read_error = read_error;
		if(size == 0)
		{
			error = std::exchange(m_read_error, {});
			if(not error)
				error = make_error_code(std::errc::io_error);
			return finish_read_error(error);
		}
	}
}

template <core_concepts::exec Exec>
template <typename Buffer>
basic_message<Buffer> stream_impl<Exec>::convert_message(
	message value, error_code &error) noexcept
{
	try
	{
		auto type = value.type;
		if constexpr(std::same_as<Buffer, std::vector<std::byte>>)
		{
			error.clear();
			return {.type = type, .body = std::move(value.body)};
		}
		else
		{
			auto body = copy_buffer_data<Buffer>(std::move(value.body));
			error.clear();
			return {.type = type, .body = std::move(body)};
		}
	}
	catch(const std::bad_alloc &)
	{
		error = make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		error = make_error_code(std::errc::io_error);
	}
	return {};
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_read_message(Handler &&handler)
{
	auto completion = asio::any_completion_handler<void(error_code, message)>(
		std::forward<Handler>(handler));
	auto self = this->shared_from_this();
	if(self->m_state != connection_state::open or self->m_read_active)
	{
		auto error = self->m_read_active ? make_error_code(std::errc::operation_in_progress) : self->m_state == connection_state::failed ? self->m_error
			: self->m_state == connection_state::closed																					 ? error_code(asio::error::eof)
			: self->m_state == connection_state::closing																				 ? make_error_code(errc::closing)
																																		 : make_error_code(errc::not_open);
		asio::post(self->m_exec,
			[handler = std::move(completion), error]() mutable
			{
				std::move(handler)(error, message{});
			});
		return;
	}

	std::shared_ptr<read_wait_operation> waiter;
	try
	{
		auto associated_allocator =
			asio::get_associated_allocator(completion);
		using waiter_allocator_t = std::allocator_traits<
			decltype(associated_allocator)>::template rebind_alloc<read_wait_operation>;
		waiter = std::allocate_shared<read_wait_operation>(
			waiter_allocator_t(associated_allocator));
		waiter->id = ++self->m_next_read_waiter_id;
		waiter->completion = std::move(completion);
		self->m_read_waiter = waiter;

		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id](asio::cancellation_type type) noexcept
				{
				if( type == asio::cancellation_type::none )
				return;
				if( auto locked = weak.lock() )
				{
					try
					{
						asio::dispatch(locked->m_exec, [locked, id, type] {
							locked->cancel_read_waiter(id, type);
						});
					}
					catch(...)
					{
					}
				} });
		}
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		if(waiter)
			self->complete_read_waiter(error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, message{});
				});
		return;
	}

	self->m_read_active = true;
	try
	{
		asio::co_spawn(self->m_exec, [self]() -> awaitable<std::tuple<error_code, message>>
			{
			for(;;)
			{
				if( self->m_local_close_initiated and
				self->m_state == connection_state::closing )
				co_return std::tuple<error_code,message> {
					make_error_code(errc::closing), {}};

				while( self->available_read_data().size() != 0 )
				{
					auto event = self->consume_frame_data();
					if( not event )
					{
						self->begin_protocol_failure(event.error());
						co_return std::tuple<error_code,message> {event.error(), {}};
					}
					if( not *event )
					continue;

					auto &value = **event;
					if( value.data )
					co_return std::tuple<error_code,message> {
						{}, std::move(*value.data)};

					if( value.op == opcode::ping or value.op == opcode::pong )
					{
						if( value.op == opcode::ping and self->m_config.automatic_pong )
						{
							auto queued = self->queue_automatic_pong(value.control);
							if( not queued )
							{
								self->fail(queued.error());
								co_return std::tuple<error_code,message> {
									queued.error(), {}};
							}
						}
						self->remember_control(value.op, std::move(value.control));
						continue;
					}

					if( value.op == opcode::close )
					{
						auto closing = self->begin_peer_close(value.control);
						if( not closing )
						{
							self->begin_protocol_failure(closing.error());
							co_return std::tuple<error_code,message> {
								closing.error(), {}};
						}
						co_return std::tuple<error_code,message> {asio::error::eof, {}};
					}
				}

				if( self->m_read_error )
				{
					auto error = std::exchange(self->m_read_error, {});
					if( error == asio::error::eof )
					{
						error_code close_error;
						self->close_transport(close_error);
						if( close_error )
						{
							self->fail(close_error);
							co_return std::tuple<error_code,message> {close_error, {}};
						}
						self->m_state = connection_state::closed;
						self->m_close_result = self->retained_close_info(false);
						self->stop_control_observer(asio::error::eof);
						self->complete_close_waiters(asio::error::eof);
						co_return std::tuple<error_code,message> {error, {}};
					}
					if( error != asio::error::operation_aborted )
					self->fail(error);
					co_return std::tuple<error_code,message> {error, {}};
				}

				auto owner = self->m_read_buffer;
				auto [error, size] = co_await http::detail::initiate_connection_io(
				[self, owner](auto next_handler) mutable {
					self->m_connection->read(mutable_buffer(
					owner->data(), owner->size()),
					asio::any_completion_handler<void(error_code,size_t)>(
					std::move(next_handler)));
				}, asio::as_tuple(asio::use_awaitable));
				if( size > owner->size() )
				{
					error = make_error_code(std::errc::io_error);
					self->fail(error);
					co_return std::tuple<error_code,message> {error, {}};
				}
				self->m_read_size = size;
				self->m_read_offset = 0;
				self->m_read_error = error;
				if( size == 0 )
				{
					if( not error )
					error = make_error_code(std::errc::io_error);
					self->m_read_error.clear();
					if( error == asio::error::eof )
					{
						error_code close_error;
						self->close_transport(close_error);
						if( close_error )
						{
							self->fail(close_error);
							co_return std::tuple<error_code,message> {close_error, {}};
						}
						self->m_state = connection_state::closed;
						self->m_close_result = self->retained_close_info(false);
						self->stop_control_observer(asio::error::eof);
						self->complete_close_waiters(asio::error::eof);
					}
					else if( error != asio::error::operation_aborted )
					self->fail(error);
					co_return std::tuple<error_code,message> {error, {}};
				}
			} }, asio::bind_executor(self->m_exec, asio::bind_cancellation_slot(waiter->cancellation.slot(), [self, waiter](const std::exception_ptr &exception, std::tuple<error_code, message> result) mutable
													   {
			ignore_unused(waiter);
			self->m_read_active = false;
			if( auto error = exception_error(exception) )
			{
				if( error != asio::error::operation_aborted )
				self->fail(error);
				self->complete_read_waiter(error);
			}
			else
			{
				auto [result_error, value] = std::move(result);
				self->complete_read_waiter(result_error, std::move(value));
			}
			if( self->m_local_close_initiated and
			self->m_state == connection_state::closing )
			self->start_close_receive(); })));
	}
	catch(...)
	{
		self->m_read_active = false;
		self->complete_read_waiter(
			exception_error(std::current_exception()));
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::complete_read_waiter(
	error_code error, message value, bool clear_slot) noexcept
{
	if(not m_read_waiter)
		return;
	auto waiter = std::exchange(m_read_waiter, {});
	if(clear_slot)
	{
		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
			slot.clear();
	}
	try
	{
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, std::move(value));
	}
	catch(...)
	{
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel_read_waiter(
	uint64_t id, asio::cancellation_type type) noexcept
{
	if(not m_read_waiter or m_read_waiter->id != id)
		return;
	try
	{
		m_read_waiter->cancellation.emit(type);
	}
	catch(...)
	{
	}
}

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_IPP
