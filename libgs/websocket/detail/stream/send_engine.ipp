// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_H
# error "Include <libgs/websocket/detail/stream/send_engine.h> instead."
#endif

namespace libgs::websocket
{

template <typename Owner>
detail::send_engine<Owner>::send_engine(Owner &owner) noexcept :
	m_owner(owner)
{

}

template <typename Owner>
void detail::send_engine<Owner>::reset(role local_role, const stream_config &config) noexcept
{
	m_frame_builder.reset(local_role, config);
	m_max_queued_write_bytes = config.max_queued_write_bytes;
	m_max_queued_write_operations = config.max_queued_write_operations;
}

template <typename Owner>
auto detail::send_engine<Owner>::prepare_control
(opcode op, const const_buffer &payload, bool borrow_payload) const noexcept -> sys_expected<prepared_frame>
{
	return m_frame_builder.prepare_control(op, payload, borrow_payload);
}

template <typename Owner>
auto detail::send_engine<Owner>::prepare_close(const close_frame &frame)
	const noexcept -> sys_expected<prepared_frame>
{
	return m_frame_builder.prepare_close(frame);
}

template <typename Owner>
auto detail::send_engine<Owner>::prepare_message(message_type type, std::span<const const_buffer> buffers)
	const noexcept -> sys_expected<std::vector<prepared_frame>>
{
	return m_frame_builder.prepare_message(type, buffers);
}

template <typename Owner>
bool detail::send_engine<Owner>::busy() const noexcept
{
	return m_wire_write_active or m_current_data or
		   not m_data_write_queue.empty() or not m_control_write_queue.empty() or
		   m_pending_auto_pong or m_pending_local_close or
		   m_pending_close_response or m_pending_protocol_close;
}

template <typename Owner>
bool detail::send_engine<Owner>::wire_write_active() const noexcept
{
	return m_wire_write_active;
}

template <typename Owner>
bool detail::send_engine<Owner>::current_data_active() const noexcept
{
	return static_cast<bool>(m_current_data);
}

template <typename Owner>
bool detail::send_engine<Owner>::ready_for_sync_protocol_write() const noexcept
{
	return not m_wire_write_active and not m_current_data;
}

template <typename Owner>
bool detail::send_engine<Owner>::queue_has_capacity(send_kind kind, size_t payload_size) const noexcept
{
	if( m_max_queued_write_operations == 0 or m_max_queued_write_bytes == 0 )
		return false;

	if( m_queued_write_operations >= m_max_queued_write_operations )
		return false;

	if( kind == send_kind::data and
		payload_size > m_max_queued_write_bytes -
			std::min(m_queued_write_bytes, m_max_queued_write_bytes) )
		return false;
	return true;
}

template <typename Owner>
void detail::send_engine<Owner>::remember_write_error(uint64_t sequence, error_code error) noexcept
{
	if( error and not m_unobserved_write_error )
		m_unobserved_write_error = std::pair{sequence, error};
}

template <typename Owner>
error_code detail::send_engine<Owner>::observe_write_error(uint64_t target) noexcept
{
	if( m_unobserved_write_error and m_unobserved_write_error->first <= target )
	{
		auto error = m_unobserved_write_error->second;
		m_unobserved_write_error.reset();
		return error;
	}
	return {};
}

template <typename Owner>
void detail::send_engine<Owner>::complete_write_waiters()
{
	if( m_current_data )
		return ;

	while( not m_write_waiters.empty() and m_write_waiters.front()->target <= m_completed_write_sequence )
	{
		auto waiter = std::move(m_write_waiters.front());
		m_write_waiters.pop_front();

		auto error = observe_write_error(waiter->target);
		deliver_write_waiter(std::move(waiter), error);
	}
}

template <typename Owner>
void detail::send_engine<Owner>::deliver_write_waiter
(const std::shared_ptr<write_waiter> &waiter, error_code error, bool clear_slot) noexcept
{
	if( not waiter or not waiter->completion )
		return ;

	if( clear_slot )
	{
		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if( slot.is_connected() )
			slot.clear();
	}
	try {
		auto completion = std::move(waiter->completion);
		std::move(completion)(error);
	}
	catch(...) {}
}

template <typename Owner>
void detail::send_engine<Owner>::cancel_write_waiter(uint64_t id) noexcept
{
	for(auto iterator = m_write_waiters.begin(); iterator != m_write_waiters.end(); ++iterator)
	{
		if( (*iterator)->id != id )
			continue;

		auto waiter = std::move(*iterator);
		m_write_waiters.erase(iterator);

		deliver_write_waiter(std::move(waiter), asio::error::operation_aborted, false);
		return ;
	}
}

template <typename Owner>
void detail::send_engine<Owner>::deliver_send_completion
(const std::shared_ptr<send_operation> &operation, error_code error) noexcept
{
	if( not operation->completion )
		return ;

	auto slot = asio::get_associated_cancellation_slot(operation->completion);
	if( slot.is_connected() )
		slot.clear();
	try {
		auto completion = std::move(operation->completion);
		std::move(completion)(error, operation->transferred);
	}
	catch(...) {}
}

template <typename Owner>
void detail::send_engine<Owner>::complete_send_operation
(const std::shared_ptr<send_operation> &operation, error_code error) noexcept
{
	if( operation->kind == send_kind::data )
	{
		m_completed_write_sequence = std::max(m_completed_write_sequence, operation->sequence);
		remember_write_error(operation->sequence, error);
	}
	deliver_send_completion(operation, error);

	if( operation->kind == send_kind::data )
		complete_write_waiters();
}

template <typename Owner>
void detail::send_engine<Owner>::install_send_cancellation(const std::shared_ptr<send_operation> &operation)
{
	auto slot = asio::get_associated_cancellation_slot(operation->completion);
	if( not slot.is_connected() )
		return ;

	slot.assign([weak = m_owner.weak_from_this(), id = operation->id](asio::cancellation_type type) noexcept
	{
		if( type == asio::cancellation_type::none )
			return ;

		if( auto self = weak.lock() )
		{
			try {
				asio::dispatch(self->executor(), [self, id]{
					self->send_side().cancel_queued_send(id);
				});
			}
			catch(...) {}
		}
	});
}

template <typename Owner>
void detail::send_engine<Owner>::cancel_queued_send(uint64_t id) noexcept
{
	for(auto &operation : m_data_write_queue)
	{
		if( operation->id != id or operation->cancel_requested )
			continue;

		operation->cancel_requested = true;
		// Keep a bounded tombstone in sequence order until earlier data writes
		// finish. This lets wait_written() preserve snapshot ordering without
		// allowing cancellation churn to bypass the queue limits.
		deliver_send_completion(operation, asio::error::operation_aborted);
		return ;
	}
	for(auto iterator = m_control_write_queue.begin(); iterator != m_control_write_queue.end(); ++iterator)
	{
		if( (*iterator)->id != id )
			continue;

		auto operation = std::move(*iterator);
		m_control_write_queue.erase(iterator);

		if( operation->queued_counted )
		{
			m_queued_write_operations--;
			operation->queued_counted = false;
		}
		deliver_send_completion(operation, asio::error::operation_aborted);
		return ;
	}
}

template <typename Owner>
void detail::send_engine<Owner>::fail_queued_controls(error_code error)
{
	while(not m_control_write_queue.empty())
	{
		auto operation = std::move(m_control_write_queue.front());
		m_control_write_queue.pop_front();

		if( operation->queued_counted )
			m_queued_write_operations--;

		complete_send_operation(operation, error);
	}
}

template <typename Owner>
void detail::send_engine<Owner>::fail_queued_writes(error_code error)
{
	fail_queued_controls(error);
	while(not m_data_write_queue.empty())
	{
		auto operation = std::move(m_data_write_queue.front());
		m_data_write_queue.pop_front();

		if( operation->queued_counted )
		{
			m_queued_write_operations--;
			m_queued_write_bytes -= operation->queued_payload_size;
		}
		complete_send_operation(operation, operation->cancel_requested ?
			error_code(asio::error::operation_aborted) : error
		);
	}
}

template <typename Owner>
void detail::send_engine<Owner>::fail_current_if_idle(error_code error) noexcept
{
	if( m_wire_write_active or not m_current_data )
		return ;

	auto operation = std::exchange(m_current_data, {});
	complete_send_operation(operation, error);
}

template <typename Owner>
void detail::send_engine<Owner>::clear_automatic_pong() noexcept
{
	m_pending_auto_pong.reset();
}

template <typename Owner>
void detail::send_engine<Owner>::clear_local_close() noexcept
{
	m_pending_local_close.reset();
}

template <typename Owner>
void detail::send_engine<Owner>::clear_close_response() noexcept
{
	m_pending_close_response.reset();
}

template <typename Owner>
void detail::send_engine<Owner>::clear_protocol_close() noexcept
{
	m_pending_protocol_close.reset();
}

template <typename Owner>
void detail::send_engine<Owner>::clear_protocol_frames() noexcept
{
	clear_automatic_pong();
	clear_local_close();
	clear_close_response();
	clear_protocol_close();
}

template <typename Owner>
void detail::send_engine<Owner>::queue_local_close(prepared_frame frame)
{
	m_pending_local_close = std::move(frame);
}

template <typename Owner>
void detail::send_engine<Owner>::queue_protocol_close(prepared_frame frame)
{
	m_pending_protocol_close = std::move(frame);
}

template <typename Owner>
bool detail::send_engine<Owner>::has_local_close() const noexcept
{
	return m_pending_local_close.has_value();
}

template <typename Owner>
sys_expected<> detail::send_engine<Owner>::queue_close_response
(const std::vector<std::byte> &payload) noexcept
{
	return retain_protocol_payload(m_pending_close_response, payload);
}

template <typename Owner>
void detail::send_engine<Owner>::start_wire_frame
(prepared_frame frame, wire_frame_kind kind, std::shared_ptr<send_operation> operation) noexcept
{
	m_wire_write_active = true;
	m_owner.start_transport_write(std::move(frame), kind, std::move(operation));
}

template <typename Owner>
void detail::send_engine<Owner>::complete_wire_frame(const prepared_frame &frame, wire_frame_kind kind,
	std::shared_ptr<send_operation> operation, error_code error, size_t wire_size) noexcept
{
	m_wire_write_active = false;
	const bool shutdown = not m_owner.send_transport_ready();

	const auto payload_size = wire_size > frame.header_size ?
		std::min(frame.payload_size, wire_size - frame.header_size) : 0;

	if( operation )
		operation->transferred += payload_size;

	if( not error and wire_size != frame.header_size + frame.payload_size )
		error = make_error_code(std::errc::io_error);

	if( shutdown and not error )
		error = asio::error::operation_aborted;

	if( error )
	{
		const auto completion_error = m_owner.protocol_failure_error(error);

		if( operation )
		{
			if( operation->kind == send_kind::data )
				m_current_data.reset();
			complete_send_operation(operation, completion_error);
		}
		if( m_current_data )
		{
			auto data = std::exchange(m_current_data, {});
			complete_send_operation(data, completion_error);
		}
		fail_queued_writes(completion_error);
		m_pending_auto_pong.reset();
		m_pending_close_response.reset();

		if( not shutdown )
			m_owner.handle_send_failure(error);
		return ;
	}
	switch(kind)
	{
	case wire_frame_kind::data:
		m_last_wire_was_control = false;
		operation->frame_index++;

		if( m_owner.protocol_failure_active() )
		{
			m_current_data.reset();
			complete_send_operation(operation,
				operation->frame_index == operation->frames.size() ?
				error_code{} : m_owner.protocol_failure_error({})
			);
			schedule();
			return ;
		}
		if( operation->frame_index == operation->frames.size() )
		{
			m_current_data.reset();
			complete_send_operation(operation, {});
		}
		break;

	case wire_frame_kind::application_control:
		m_last_wire_was_control = true;
		complete_send_operation(operation, {});
		break;

	case wire_frame_kind::automatic_pong:
		m_last_wire_was_control = true;
		break;

	case wire_frame_kind::local_close:
	case wire_frame_kind::close_response:
	case wire_frame_kind::protocol_close:
		m_last_wire_was_control = true;
		m_owner.handle_wire_frame_sent(kind);
		return ;
	}
	schedule();
}

template <typename Owner>
void detail::send_engine<Owner>::schedule()
{
	if( m_wire_write_active or not m_owner.send_transport_ready() )
		return ;

	if( m_pending_protocol_close and not m_current_data )
	{
		auto frame = std::exchange(m_pending_protocol_close, nullopt).value();
		start_wire_frame(std::move(frame), wire_frame_kind::protocol_close);
		return ;
	}
	if( m_pending_close_response and not m_current_data )
	{
		auto payload = std::exchange(m_pending_close_response, nullopt);
		m_pending_auto_pong.reset();

		auto frame = m_frame_builder.prepare_control(opcode::close,
			const_buffer(payload->data(), payload->size())
		);
		if( not frame )
		{
			fail_queued_writes(frame.error());
			m_owner.handle_send_failure(frame.error());
			return ;
		}
		start_wire_frame(std::move(*frame), wire_frame_kind::close_response);
		return ;
	}
	if( m_pending_local_close and not m_current_data and m_data_write_queue.empty() )
	{
		auto frame = std::exchange(m_pending_local_close, nullopt).value();
		m_pending_auto_pong.reset();

		fail_queued_controls(make_error_code(errc::closing));
		start_wire_frame(std::move(frame), wire_frame_kind::local_close);
		return ;
	}
	const bool data_available = m_current_data or not m_data_write_queue.empty();

	if( (not m_last_wire_was_control or not data_available) and
		m_pending_auto_pong and not m_pending_local_close and
		not m_pending_close_response )
	{
		auto payload = std::exchange(m_pending_auto_pong, nullopt);
		auto frame = m_frame_builder.prepare_control(opcode::pong,
			const_buffer(payload->data(), payload->size())
		);
		if( not frame )
		{
			fail_queued_writes(frame.error());
			m_owner.handle_send_failure(frame.error());
			return ;
		}
		start_wire_frame(std::move(*frame), wire_frame_kind::automatic_pong);
		return ;
	}
	if( (not m_last_wire_was_control or not data_available) and
		not m_control_write_queue.empty() and not m_pending_local_close and
		not m_pending_close_response )
	{
		auto operation = std::move(m_control_write_queue.front());
		m_control_write_queue.pop_front();

		if( operation->queued_counted )
		{
			m_queued_write_operations--;
			operation->queued_counted = false;
		}
		auto frame = operation->frames.front();

		start_wire_frame(std::move(frame),
			wire_frame_kind::application_control, std::move(operation)
		);
		return ;
	}
	if( data_available )
	{
		if( not m_current_data )
		{
			while( not m_data_write_queue.empty() and m_data_write_queue.front()->cancel_requested )
			{
				auto cancelled = std::move(m_data_write_queue.front());
				m_data_write_queue.pop_front();

				if( cancelled->queued_counted )
				{
					m_queued_write_operations--;
					m_queued_write_bytes -= cancelled->queued_payload_size;
					cancelled->queued_counted = false;
				}
				complete_send_operation(cancelled, asio::error::operation_aborted);
			}
			if( m_data_write_queue.empty() )
			{
				schedule();
				return ;
			}
			m_current_data = std::move(m_data_write_queue.front());
			m_data_write_queue.pop_front();

			if( m_current_data->queued_counted )
			{
				m_queued_write_operations--;
				m_queued_write_bytes -= m_current_data->queued_payload_size;
				m_current_data->queued_counted = false;
			}
		}
		start_wire_frame(m_current_data->frames[m_current_data->frame_index],
			wire_frame_kind::data, m_current_data
		);
		return ;
	}
	// With no data pending, fairness no longer limits consecutive controls.
	m_last_wire_was_control = false;
	if( m_pending_auto_pong or m_pending_local_close or not m_control_write_queue.empty() )
		schedule();
}

template <typename Owner>
error_code detail::send_engine<Owner>::enqueue_send_operation
(std::shared_ptr<send_operation> operation) noexcept
{
	if( not busy() )
	{
		if( operation->kind == send_kind::data )
			m_current_data = std::move(operation);
		else
		{
			auto frame = operation->frames.front();
			start_wire_frame(std::move(frame),
				wire_frame_kind::application_control, std::move(operation)
			);
			return {};
		}
		schedule();
		return {};
	}
	if( not queue_has_capacity(operation->kind, operation->queued_payload_size) )
		return make_error_code(errc::write_queue_full);
	try {
		m_queued_write_operations++;
		operation->queued_counted = true;

		if( operation->kind == send_kind::data )
		{
			m_queued_write_bytes += operation->queued_payload_size;
			m_data_write_queue.push_back(operation);
		}
		else
			m_control_write_queue.push_back(operation);
		return {};
	}
	catch(const std::bad_alloc&)
	{
		m_queued_write_operations--;
		operation->queued_counted = false;

		if( operation and operation->kind == send_kind::data )
			m_queued_write_bytes -= operation->queued_payload_size;

		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		m_queued_write_operations--;
		operation->queued_counted = false;

		if( operation and operation->kind == send_kind::data )
			m_queued_write_bytes -= operation->queued_payload_size;
	}
	return make_error_code(std::errc::io_error);
}

template <typename Owner>
template <typename Handler>
void detail::send_engine<Owner>::async_write_message
(message_type type, std::span<const const_buffer> buffers,
	std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code, size_t)>(std::forward<Handler>(handler));

	auto self = m_owner.shared_from_this();
	if( auto error = self->write_state_error() )
	{
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, 0);
		});
		return ;
	}
	auto frames = self->send_side().prepare_message(type, buffers);
	if( not frames )
	{
		auto error = frames.error();
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, 0);
		});
		return ;
	}
	std::shared_ptr<send_operation> operation;
	try
	{
		operation = std::make_shared<send_operation>();

		operation->kind = send_kind::data;
		operation->frames = std::move(*frames);

		operation->payload_owner = std::move(payload_owner);
		operation->completion = std::move(completion);
		operation->id = ++self->send_side().m_next_send_operation_id;

		for(const auto &frame : operation->frames)
			operation->queued_payload_size += frame.payload_size;

		self->send_side().install_send_cancellation(operation);
		operation->sequence = ++self->send_side().m_last_write_sequence;

		if( auto error = self->send_side().enqueue_send_operation(operation) )
		{
			self->send_side().m_last_write_sequence--;
			self->send_side().deliver_send_completion(operation, error);
		}
	}
	catch(const std::bad_alloc&)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if( operation and operation->completion )
			self->send_side().deliver_send_completion(operation, error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, 0);
			});
		}
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if( operation and operation->completion )
			self->send_side().deliver_send_completion(operation, error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, 0);
			});
		}
	}
}

template <typename Owner>
template <typename Handler>
void detail::send_engine<Owner>::async_write_control
(opcode op, const const_buffer &payload, Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code, size_t)>(std::forward<Handler>(handler));

	auto self = m_owner.shared_from_this();
	if( auto error = self->write_state_error() )
	{
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, 0);
		});
		return ;
	}
	auto frame = self->send_side().prepare_control(op, payload, true);
	if( not frame )
	{
		auto error = frame.error();
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, 0);
		});
		return ;
	}
	std::shared_ptr<send_operation> operation;
	try {
		operation = std::make_shared<send_operation>();
		operation->kind = send_kind::application_control;

		operation->frames.push_back(std::move(*frame));
		operation->completion = std::move(completion);

		operation->id = ++self->send_side().m_next_send_operation_id;
		operation->queued_payload_size = operation->frames.front().payload_size;

		self->send_side().install_send_cancellation(operation);
		if( auto error = self->send_side().enqueue_send_operation(operation) )
			self->send_side().deliver_send_completion(operation, error);
	}
	catch(const std::bad_alloc&)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if( operation and operation->completion )
			self->send_side().deliver_send_completion(operation, error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, 0);
			});
		}
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if( operation and operation->completion )
			self->send_side().deliver_send_completion(operation, error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, 0);
			});
		}
	}
}

template <typename Owner>
size_t detail::send_engine<Owner>::write_control
(opcode op, const const_buffer &payload, error_code &error) noexcept
{
	error.clear();
	if( auto state_error = m_owner.write_state_error() )
	{
		error = state_error;
		return 0;
	}
	auto frame = m_frame_builder.prepare_control(op, payload, true);
	if( not frame )
	{
		error = frame.error();
		return 0;
	}
	auto transferred = m_owner.write_prepared(*frame, error);
	if( error )
		m_owner.handle_send_failure(error);
	return transferred;
}

template <typename Owner>
void detail::send_engine<Owner>::wait_written(error_code &error) noexcept
{
	if( m_completed_write_sequence < m_last_write_sequence )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return ;
	}
	error = observe_write_error(m_last_write_sequence);
}

template <typename Owner>
template <typename Handler>
void detail::send_engine<Owner>::async_wait_written(Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code)>(std::forward<Handler>(handler));

	const auto target = m_last_write_sequence;
	if( target <= m_completed_write_sequence )
	{
		auto error = observe_write_error(target);
		asio::post(m_owner.executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error);
		});
		return ;
	}
	std::shared_ptr<write_waiter> waiter;
	bool queued = false;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);

		using waiter_allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::rebind_alloc<write_waiter>;

		waiter = std::allocate_shared<write_waiter>(
			waiter_allocator_t(associated_allocator)
		);
		waiter->id = ++m_next_write_waiter_id;
		waiter->target = target;
		waiter->completion = std::move(completion);

		m_write_waiters.push_back(waiter);
		queued = true;

		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if( slot.is_connected() )
		{
			slot.assign([weak = m_owner.weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto self = weak.lock() )
				{
					try {
						asio::dispatch(self->executor(), [self, id]{
							self->send_side().cancel_write_waiter(id);
						});
					}
					catch(...) {}
				}
			});
		}
	}
	catch(const std::bad_alloc&)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if( waiter )
		{
			if( queued )
			{
				auto iterator = std::ranges::find(m_write_waiters, waiter);
				if( iterator != m_write_waiters.end() )
					m_write_waiters.erase(iterator);
			}
			deliver_write_waiter(waiter, error);
		}
		else
		{
			asio::post(m_owner.executor(),
			[handler = std::move(completion), error]() mutable {
				std::move(handler)(error);
			});
		}
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if( waiter )
		{
			if( queued )
			{
				auto iterator = std::ranges::find(m_write_waiters, waiter);
				if( iterator != m_write_waiters.end() )
					m_write_waiters.erase(iterator);
			}
			deliver_write_waiter(waiter, error);
		}
		else
		{
			asio::post(m_owner.executor(),
			[handler = std::move(completion), error]() mutable {
				std::move(handler)(error);
			});
		}
	}
}

template <typename Owner>
sys_expected<> detail::send_engine<Owner>::retain_protocol_payload
(optional<std::vector<std::byte>> &slot, const std::vector<std::byte> &payload) noexcept
{
	try {
		slot = payload;
		return make_sys_expected();
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

template <typename Owner>
sys_expected<> detail::send_engine<Owner>::queue_automatic_pong
(const std::vector<std::byte> &payload) noexcept
{
	auto retained = retain_protocol_payload(m_pending_auto_pong, payload);
	if( retained )
		schedule();
	return retained;
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_IPP
