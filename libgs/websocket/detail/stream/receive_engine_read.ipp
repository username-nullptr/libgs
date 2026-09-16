// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_READ_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_READ_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_H
# error "Include <libgs/websocket/detail/stream/receive_engine.h> instead."
#endif

namespace libgs::websocket
{

template <typename Owner>
detail::receive_engine<Owner>::receive_engine(Owner &owner) noexcept :
	m_owner(owner)
{

}

template <typename Owner>
void detail::receive_engine<Owner>::reset(role local_role, const stream_config &config,
	std::span<const extension> extensions, std::vector<std::byte> pending_data)
{
	m_buffer.reset(local_role, config, extensions, std::move(pending_data));
	m_read_error.clear();
	m_read_active = false;
}

template <typename Owner>
bool detail::receive_engine<Owner>::active() const noexcept
{
	return m_read_active;
}

template <typename Owner>
void detail::receive_engine<Owner>::set_active(bool value) noexcept
{
	m_read_active = value;
}

template <typename Owner>
detail::receive_event_result
detail::receive_engine<Owner>::next_event(receive_target target) noexcept
{
	for(;;)
	{
		while( m_buffer.available_data().size() != 0 )
		{
			auto event = m_buffer.consume(target);
			if( not event )
			{
				return {
					.error = event.error(),
					.failure_origin = receive_failure_origin::protocol,
				};
			}
			if( *event )
				return {.event = std::move(**event)};
		}
		if( m_read_error )
		{
			return {
				.error = std::exchange(m_read_error, {}),
				.failure_origin = receive_failure_origin::transport,
			};
		}
		error_code read_error;
		auto storage = m_buffer.read_storage();

		const auto size = m_owner.read_transport (
			mutable_buffer(storage->data(), storage->size()), read_error
		);
		if( auto error = m_buffer.commit_read(size) )
		{
			return {
				.error = error,
				.failure_origin = receive_failure_origin::buffer,
			};
		}
		m_read_error = read_error;
		if( size == 0 )
		{
			auto error = std::exchange(m_read_error, {});
			if( not error )
				error = make_error_code(std::errc::io_error);

			return {
				.error = error,
				.failure_origin = receive_failure_origin::transport,
			};
		}
	}
}

template <typename Owner>
awaitable<detail::receive_event_result>
detail::receive_engine<Owner>::async_next_event(receive_target target)
{
	for(;;)
	{
		while( m_buffer.available_data().size() != 0 )
		{
			auto event = m_buffer.consume(target);
			if( not event )
			{
				co_return receive_event_result {
					.error = event.error(),
					.failure_origin = receive_failure_origin::protocol,
				};
			}
			if( *event )
				co_return receive_event_result {.event = std::move(**event)};
		}
		if( m_read_error )
		{
			co_return receive_event_result {
				.error = std::exchange(m_read_error, {}),
				.failure_origin = receive_failure_origin::transport,
			};
		}
		auto storage = m_buffer.read_storage();
		auto [read_error, size] = co_await m_owner.async_read_transport(storage);

		if( auto error = m_buffer.commit_read(size) )
		{
			co_return receive_event_result {
				.error = error,
				.failure_origin = receive_failure_origin::buffer,
			};
		}
		m_read_error = read_error;
		if( size == 0 )
		{
			auto error = std::exchange(m_read_error, {});
			if( not error )
				error = make_error_code(std::errc::io_error);

			co_return receive_event_result {
				.error = error,
				.failure_origin = receive_failure_origin::transport,
			};
		}
	}
}

template <typename Owner>
message detail::receive_engine<Owner>::finish_read_error(error_code &error) noexcept
{
	if( error == asio::error::operation_aborted )
		return {};

	if( error == asio::error::eof )
	{
		error = m_owner.finish_receive_eof();
		return {};
	}
	m_owner.handle_receive_failure(error);
	return {};
}

template <typename Owner>
message detail::receive_engine<Owner>::read(error_code &error) noexcept
{
	error.clear();
	if( auto state_error = m_owner.read_state_error() )
	{
		error = state_error;
		return {};
	}
	if( auto target_error = m_buffer.target_error(receive_target::message) )
	{
		error = target_error;
		return {};
	}
	if( m_read_active )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}
	m_read_active = true;
	struct read_guard
	{
		bool &active;
		~read_guard() {
			active = false;
		}
	}
	guard{m_read_active};

	for(;;)
	{
		auto next = next_event(receive_target::message);
		if( next.error )
		{
			error = next.error;
			if( next.failure_origin == receive_failure_origin::protocol )
			{
				m_owner.handle_receive_protocol_failure(error, true);
				return {};
			}
			if( next.failure_origin == receive_failure_origin::buffer )
			{
				m_owner.handle_receive_failure(error);
				return {};
			}
			return finish_read_error(error);
		}
		auto &value = *next.event;
		if( value.data )
			return std::move(*value.data);

		if( value.op == opcode::ping or value.op == opcode::pong )
		{
			auto handled = m_owner.handle_sync_control(value.op, value.control);
			if( not handled )
			{
				error = handled.error();
				m_owner.handle_receive_failure(error);
				return {};
			}
			continue;
		}
		if( value.op == opcode::close )
		{
			auto closed = m_owner.handle_sync_peer_close(value.control);
			if( not closed )
			{
				error = closed.error();
				m_owner.handle_receive_protocol_failure(error, true);
				return {};
			}
			error = asio::error::eof;
			return {};
		}
	}
}

template <typename Owner>
data_frame detail::receive_engine<Owner>::read_frame(error_code &error) noexcept
{
	error.clear();
	if( auto state_error = m_owner.frame_read_state_error() )
	{
		error = state_error;
		return {};
	}
	if( auto target_error = m_buffer.target_error(receive_target::frame) )
	{
		error = target_error;
		return {};
	}
	if( m_read_active )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}
	m_read_active = true;
	struct read_guard
	{
		bool &active;
		~read_guard() {
			active = false;
		}
	}
	guard{m_read_active};

	for(;;)
	{
		auto next = next_event(receive_target::frame);
		if( next.error )
		{
			error = next.error;
			if( next.failure_origin == receive_failure_origin::protocol )
			{
				m_owner.handle_receive_protocol_failure(error, true);
				return {};
			}
			if( next.failure_origin == receive_failure_origin::buffer )
			{
				m_owner.handle_receive_failure(error);
				return {};
			}
			ignore_unused(finish_read_error(error));
			return {};
		}
		auto &value = *next.event;
		if( value.frame )
			return std::move(*value.frame);

		if( value.op == opcode::ping or value.op == opcode::pong )
		{
			auto handled = m_owner.handle_sync_control(value.op, value.control);
			if( not handled )
			{
				error = handled.error();
				m_owner.handle_receive_failure(error);
				return {};
			}
			continue;
		}
		if( value.op == opcode::close )
		{
			auto closed = m_owner.handle_sync_peer_close(value.control);
			if( not closed )
			{
				error = closed.error();
				m_owner.handle_receive_protocol_failure(error, true);
				return {};
			}
			error = asio::error::eof;
			return {};
		}
	}
}

template <typename Owner>
template <typename Consumer>
message_info detail::receive_engine<Owner>::consume(Consumer &&consumer, error_code &error) noexcept
{
	error.clear();
	if( auto state_error = m_owner.consume_state_error() )
	{
		error = state_error;
		return {};
	}
	if( auto target_error = m_buffer.target_error(receive_target::chunk) )
	{
		error = target_error;
		return {};
	}
	if( m_read_active )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}
	m_read_active = true;

	struct read_guard
	{
		bool &active;
		~read_guard() {
			active = false;
		}
	}
	guard{m_read_active};

	for(;;)
	{
		auto next = next_event(receive_target::chunk);
		if( next.error )
		{
			error = next.error;
			if( next.failure_origin == receive_failure_origin::protocol )
				m_owner.handle_receive_protocol_failure(error, true);
			else
				ignore_unused(finish_read_error(error));
			return {};
		}
		auto &value = *next.event;
		if( value.chunk )
		{
			try {
				std::invoke(consumer, *value.chunk);
			}
			catch(...)
			{
				error = exception_error(std::current_exception());
				m_owner.handle_receive_failure(error);
				return {};
			}
			if( value.chunk->last )
			{
				return message_info {
					.type = value.chunk->type,
					.size = value.chunk->offset + value.chunk->body.size(),
				};
			}
			continue;
		}
		if( value.op == opcode::ping or value.op == opcode::pong )
		{
			auto handled = m_owner.handle_sync_control(value.op, value.control);
			if( not handled )
			{
				error = handled.error();
				m_owner.handle_receive_failure(error);
				return {};
			}
			continue;
		}
		if( value.op == opcode::close )
		{
			auto closed = m_owner.handle_sync_peer_close(value.control);
			if( not closed )
			{
				error = closed.error();
				m_owner.handle_receive_protocol_failure(error, true);
				return {};
			}
			error = asio::error::eof;
			return {};
		}
	}
}

template <typename Owner>
void detail::receive_engine<Owner>::async_read_message(message_handler_t completion)
{
	auto self = m_owner.shared_from_this();
	auto state_error = self->read_state_error();

	if( not state_error )
		state_error = self->receive_side().m_buffer.target_error(receive_target::message);

	if( state_error or self->receive_side().m_read_active )
	{
		auto error = self->receive_side().m_read_active ?
			make_error_code(std::errc::operation_in_progress) : state_error;

		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, message{});
		});
		return ;
	}
	std::shared_ptr<read_wait_operation> waiter;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);

		using waiter_allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::template rebind_alloc<read_wait_operation>;

		waiter = std::allocate_shared
			<read_wait_operation>(waiter_allocator_t(associated_allocator));

		waiter->id = ++self->receive_side().m_next_read_waiter_id;
		waiter->completion = std::move(completion);
		self->receive_side().m_read_waiter = waiter;

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto locked = weak.lock() )
				{
					try {
						asio::dispatch(locked->executor(), [locked, id, type]{
							locked->receive_side().cancel_read_waiter(id, type);
						});
					}
					catch(...) {}
				}
			});
		}
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		if( waiter )
			self->receive_side().complete_read_waiter(error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, message{});
			});
		}
		return ;
	}
	self->receive_side().m_read_active = true;
	try
	{
		asio::co_spawn(self->executor(), [self]() -> awaitable<std::tuple<error_code, message>>
		{
			for(;;)
			{
				if( self->close_receive_pending() )
					co_return std::tuple<error_code,message>{make_error_code(errc::closing), {}};

				auto next = co_await self->receive_side().async_next_event (
					receive_target::message
				);
				if( next.error )
				{
					if( next.failure_origin == receive_failure_origin::protocol )
					{
						self->handle_receive_protocol_failure(next.error);
						co_return std::tuple<error_code,message>{next.error, {}};
					}
					if( next.failure_origin == receive_failure_origin::buffer )
					{
						self->handle_receive_failure(next.error);
						co_return std::tuple<error_code,message>{next.error, {}};
					}
					auto error = next.error;
					if( error == asio::error::eof )
						error = self->finish_receive_eof();
					else if( error != asio::error::operation_aborted )
						self->handle_receive_failure(error);

					co_return std::tuple<error_code,message>{error, {}};
				}
				auto &value = *next.event;
				if( value.data )
					co_return std::tuple<error_code,message>{error_code{}, std::move(*value.data)};

				if( value.op == opcode::ping or value.op == opcode::pong )
				{
					auto control_error = co_await
						self->handle_async_control(value.op, value.control);

					if( control_error )
					{
						if( control_error != asio::error::operation_aborted )
							self->handle_receive_failure(control_error);
						co_return std::tuple<error_code,message>{control_error, {}};
					}
					continue;
				}
				if( value.op == opcode::close )
				{
					auto closing = self->begin_peer_close(value.control);
					if( not closing )
					{
						self->handle_receive_protocol_failure(closing.error());
						co_return std::tuple<error_code,message>{closing.error(), {}};
					}
					co_return std::tuple<error_code,message>{asio::error::eof, {}};
				}
			}
		},
		asio::bind_executor(self->executor(), asio::bind_cancellation_slot(waiter->cancellation.slot(),
		[self, waiter](const std::exception_ptr &exception, std::tuple<error_code, message> result) mutable
		{
			ignore_unused(waiter);
			self->receive_side().m_read_active = false;

			if( auto error = exception_error(exception) )
			{
				if( error != asio::error::operation_aborted )
					self->handle_receive_failure(error);
				self->receive_side().complete_read_waiter(error);
			}
			else
			{
				auto [result_error, value] = std::move(result);
				self->receive_side().complete_read_waiter(result_error, std::move(value));
			}
			if( self->close_receive_pending() )
				self->start_close_receive(); })
		));
	}
	catch(...)
	{
		self->receive_side().m_read_active = false;
		self->receive_side().complete_read_waiter(exception_error(std::current_exception()));
	}
}

template <typename Owner>
void detail::receive_engine<Owner>::async_read_frame(frame_handler_t completion)
{
	auto self = m_owner.shared_from_this();
	auto state_error = self->frame_read_state_error();

	if( not state_error )
		state_error = self->receive_side().m_buffer.target_error(receive_target::frame);

	if( state_error or self->receive_side().m_read_active )
	{
		auto error = self->receive_side().m_read_active ?
			make_error_code(std::errc::operation_in_progress) : state_error;

		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, data_frame{});
		});
		return ;
	}
	std::shared_ptr<frame_read_wait_operation> waiter;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);

		using waiter_allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::template rebind_alloc<frame_read_wait_operation>;

		waiter = std::allocate_shared
			<frame_read_wait_operation>(waiter_allocator_t(associated_allocator));

		waiter->id = ++self->receive_side().m_next_read_waiter_id;
		waiter->completion = std::move(completion);
		self->receive_side().m_frame_read_waiter = waiter;

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto locked = weak.lock() )
				{
					try {
						asio::dispatch(locked->executor(), [locked, id, type]{
							locked->receive_side().cancel_read_waiter(id, type);
						});
					}
					catch(...) {}
				}
			});
		}
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		if( waiter )
			self->receive_side().complete_frame_read_waiter(error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, data_frame{});
			});
		}
		return ;
	}
	self->receive_side().m_read_active = true;
	try
	{
		asio::co_spawn(self->executor(), [self]() -> awaitable<std::tuple<error_code, data_frame>>
		{
			for(;;)
			{
				if( self->close_receive_pending() )
					co_return std::tuple<error_code,data_frame>{make_error_code(errc::closing), {}};

				auto next = co_await self->receive_side().async_next_event (
					receive_target::frame
				);
				if( next.error )
				{
					if( next.failure_origin == receive_failure_origin::protocol )
					{
						self->handle_receive_protocol_failure(next.error);
						co_return std::tuple<error_code,data_frame>{next.error, {}};
					}
					if( next.failure_origin == receive_failure_origin::buffer )
					{
						self->handle_receive_failure(next.error);
						co_return std::tuple<error_code,data_frame>{next.error, {}};
					}
					auto error = next.error;
					if( error == asio::error::eof )
						error = self->finish_receive_eof();

					else if( error != asio::error::operation_aborted )
						self->handle_receive_failure(error);

					co_return std::tuple<error_code,data_frame>{error, {}};
				}
				auto &value = *next.event;
				if( value.frame )
					co_return std::tuple<error_code,data_frame>{error_code{}, std::move(*value.frame)};

				if( value.op == opcode::ping or value.op == opcode::pong )
				{
					auto control_error = co_await
						self->handle_async_control(value.op, value.control);

					if( control_error )
					{
						if( control_error != asio::error::operation_aborted )
							self->handle_receive_failure(control_error);
						co_return std::tuple<error_code,data_frame>{control_error, {}};
					}
					continue;
				}
				if( value.op == opcode::close )
				{
					auto closing = self->begin_peer_close(value.control);
					if( not closing )
					{
						self->handle_receive_protocol_failure(closing.error());
						co_return std::tuple<error_code,data_frame>{closing.error(), {}};
					}
					co_return std::tuple<error_code,data_frame>{asio::error::eof, {}};
				}
			}
		},
		asio::bind_executor(self->executor(), asio::bind_cancellation_slot(waiter->cancellation.slot(),
		[self, waiter](const std::exception_ptr &exception, std::tuple<error_code, data_frame> result) mutable
		{
			ignore_unused(waiter);
			self->receive_side().m_read_active = false;

			if( auto error = exception_error(exception) )
			{
				if( error != asio::error::operation_aborted )
					self->handle_receive_failure(error);
				self->receive_side().complete_frame_read_waiter(error);
			}
			else
			{
				auto [result_error, value] = std::move(result);
				self->receive_side().complete_frame_read_waiter (
					result_error, std::move(value)
				);
			}
			if( self->close_receive_pending() )
				self->start_close_receive(); })
		));
	}
	catch(...)
	{
		self->receive_side().m_read_active = false;
		self->receive_side().complete_frame_read_waiter (
			exception_error(std::current_exception())
		);
	}
}

template <typename Owner>
template <typename Consumer>
void detail::receive_engine<Owner>::async_consume
(Consumer &&consumer, info_handler_t completion)
{
	auto self = m_owner.shared_from_this();
	auto state_error = self->consume_state_error();

	if( not state_error )
		state_error = self->receive_side().m_buffer.target_error(receive_target::chunk);

	if( state_error or self->receive_side().m_read_active )
	{
		auto error = self->receive_side().m_read_active ?
			make_error_code(std::errc::operation_in_progress) : state_error;

		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, message_info{});
		});
		return ;
	}
	std::shared_ptr<consume_wait_operation> waiter;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);

		using waiter_allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::template rebind_alloc<consume_wait_operation>;

		waiter = std::allocate_shared
			<consume_wait_operation>(waiter_allocator_t(associated_allocator));

		waiter->id = ++self->receive_side().m_next_read_waiter_id;
		waiter->completion = std::move(completion);
		self->receive_side().m_consume_waiter = waiter;

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto locked = weak.lock() )
				{
					try {
						asio::dispatch(locked->executor(), [locked, id, type]{
							locked->receive_side().cancel_read_waiter(id, type);
						});
					}
					catch(...) {}
				}
			});
		}
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		if( waiter )
			self->receive_side().complete_consume_waiter(error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, message_info{});
			});
		}
		return ;
	}
	self->receive_side().m_read_active = true;
	try
	{
		using consumer_t = std::remove_cvref_t<Consumer>;
		asio::co_spawn(self->executor(),
		[self, consumer = consumer_t(std::forward<Consumer>(consumer))]
		() mutable -> awaitable<std::tuple<error_code, message_info>>
		{
			for(;;)
			{
				if( self->close_receive_pending() )
					co_return std::tuple<error_code,message_info>{make_error_code(errc::closing), {}};

				auto next = co_await self->receive_side().async_next_event (
					receive_target::chunk
				);
				if( next.error )
				{
					if( next.failure_origin == receive_failure_origin::protocol )
					{
						self->handle_receive_protocol_failure(next.error);
						co_return std::tuple<error_code,message_info>{next.error, {}};
					}
					if( next.failure_origin == receive_failure_origin::buffer )
					{
						self->handle_receive_failure(next.error);
						co_return std::tuple<error_code,message_info>{next.error, {}};
					}
					auto error = next.error;
					if( error == asio::error::eof )
						error = self->finish_receive_eof();

					else if( error != asio::error::operation_aborted )
						self->handle_receive_failure(error);

					co_return std::tuple<error_code,message_info>{error, {}};
				}
				auto &value = *next.event;
				if( value.chunk )
				{
					std::invoke(consumer, *value.chunk);
					if( value.chunk->last )
					{
						co_return std::tuple<error_code,message_info>{error_code{}, {
							.type = value.chunk->type,
							.size = value.chunk->offset + value.chunk->body.size(),
						}};
					}
					continue;
				}
				if( value.op == opcode::ping or value.op == opcode::pong )
				{
					auto control_error = co_await
						self->handle_async_control(value.op, value.control);

					if( control_error )
					{
						if( control_error != asio::error::operation_aborted )
							self->handle_receive_failure(control_error);
						co_return std::tuple<error_code,message_info>{control_error, {}};
					}
					continue;
				}
				if( value.op == opcode::close )
				{
					auto closing = self->begin_peer_close(value.control);
					if( not closing )
					{
						self->handle_receive_protocol_failure(closing.error());
						co_return std::tuple<error_code,message_info>{closing.error(), {}};
					}
					co_return std::tuple<error_code,message_info>{asio::error::eof, {}};
				}
			}
		},
		asio::bind_executor(self->executor(), asio::bind_cancellation_slot(waiter->cancellation.slot(),
		[self, waiter](const std::exception_ptr &exception, std::tuple<error_code, message_info> result) mutable
		{
			ignore_unused(waiter);
			self->receive_side().m_read_active = false;

			if( auto error = exception_error(exception) )
			{
				if( error != asio::error::operation_aborted )
					self->handle_receive_failure(error);
				self->receive_side().complete_consume_waiter(error);
			}
			else
			{
				auto [result_error, value] = std::move(result);
				self->receive_side().complete_consume_waiter(result_error, value);
			}
			if( self->close_receive_pending() )
				self->start_close_receive(); })
		));
	}
	catch(...)
	{
		self->receive_side().m_read_active = false;
		self->receive_side().complete_consume_waiter (
			exception_error(std::current_exception())
		);
	}
}

template <typename Owner>
void detail::receive_engine<Owner>::complete_read_waiter
(error_code error, message value, bool clear_slot) noexcept
{
	if( not m_read_waiter )
	{
		if( error and m_frame_read_waiter )
			complete_frame_read_waiter(error, {}, clear_slot);
		else if( error )
			complete_consume_waiter(error, {}, clear_slot);
		return ;
	}
	auto waiter = std::exchange(m_read_waiter, {});
	if( clear_slot )
	{
		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
			slot.clear();
	}
	try {
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, std::move(value));
	}
	catch(...) {}
}

template <typename Owner>
void detail::receive_engine<Owner>::complete_frame_read_waiter
(error_code error, data_frame value, bool clear_slot) noexcept
{
	if( not m_frame_read_waiter )
		return ;

	auto waiter = std::exchange(m_frame_read_waiter, {});
	if( clear_slot )
	{
		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
			slot.clear();
	}
	try {
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, std::move(value));
	}
	catch(...) {}
}

template <typename Owner>
void detail::receive_engine<Owner>::complete_consume_waiter
(error_code error, message_info value, bool clear_slot) noexcept
{
	if( not m_consume_waiter )
		return ;

	auto waiter = std::exchange(m_consume_waiter, {});
	if( clear_slot )
	{
		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
			slot.clear();
	}
	try {
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, value);
	}
	catch(...) {}
}

template <typename Owner>
void detail::receive_engine<Owner>::cancel_read_waiter(uint64_t id, asio::cancellation_type type) noexcept
{
	try {
		if( m_read_waiter and m_read_waiter->id == id )
			m_read_waiter->cancellation.emit(type);

		else if( m_frame_read_waiter and m_frame_read_waiter->id == id )
			m_frame_read_waiter->cancellation.emit(type);

		else if( m_consume_waiter and m_consume_waiter->id == id )
			m_consume_waiter->cancellation.emit(type);
	}
	catch(...) {}
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_READ_IPP
