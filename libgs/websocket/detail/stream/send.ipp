// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_IPP

// Send scheduling, backpressure and write observers.

template <core_concepts::exec Exec>
bool stream_impl<Exec>::send_engine_busy() const noexcept
{
	return m_wire_write_active or m_current_data or
		not m_data_write_queue.empty() or not m_control_write_queue.empty() or
		m_pending_auto_pong or m_pending_local_close or
		m_pending_close_response or m_pending_protocol_close;
}

template <core_concepts::exec Exec>
error_code stream_impl<Exec>::state_write_error() const noexcept
{
	if(m_state == connection_state::failed)
		return m_error ? m_error : make_error_code(std::errc::io_error);
	if(m_state == connection_state::closed)
		return make_error_code(errc::closed);
	if(m_state == connection_state::closing)
		return m_peer_close ? make_error_code(std::errc::broken_pipe) : make_error_code(errc::closing);
	return make_error_code(errc::not_open);
}

template <core_concepts::exec Exec>
bool stream_impl<Exec>::queue_has_capacity(send_kind kind,
	size_t payload_size) const noexcept
{
	if(m_config.max_queued_write_operations == 0 or
		m_config.max_queued_write_bytes == 0)
		return false;
	if(m_queued_write_operations >= m_config.max_queued_write_operations)
		return false;
	if(kind == send_kind::data and
		payload_size > m_config.max_queued_write_bytes -
				std::min(m_queued_write_bytes, m_config.max_queued_write_bytes))
		return false;
	return true;
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::remember_write_error(
	uint64_t sequence, error_code error) noexcept
{
	if(error and not m_unobserved_write_error)
		m_unobserved_write_error = std::pair<uint64_t, error_code>{
			sequence, error};
}

template <core_concepts::exec Exec>
error_code stream_impl<Exec>::observe_write_error(uint64_t target) noexcept
{
	if(m_unobserved_write_error and
		m_unobserved_write_error->first <= target)
	{
		auto error = m_unobserved_write_error->second;
		m_unobserved_write_error.reset();
		return error;
	}
	return {};
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::complete_write_waiters()
{
	if(m_current_data)
		return;
	while(not m_write_waiters.empty() and
		m_write_waiters.front()->target <= m_completed_write_sequence)
	{
		auto waiter = std::move(m_write_waiters.front());
		m_write_waiters.pop_front();
		auto error = observe_write_error(waiter->target);
		deliver_write_waiter(std::move(waiter), error);
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::deliver_write_waiter(
	std::shared_ptr<write_waiter> waiter, error_code error,
	bool clear_slot) noexcept
{
	if(not waiter or not waiter->completion)
		return;
	if(clear_slot)
	{
		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
			slot.clear();
	}
	try
	{
		auto completion = std::move(waiter->completion);
		std::move(completion)(error);
	}
	catch(...)
	{
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel_write_waiter(uint64_t id) noexcept
{
	for(auto iterator = m_write_waiters.begin();
		iterator != m_write_waiters.end(); ++iterator)
	{
		if((*iterator)->id != id)
			continue;
		auto waiter = std::move(*iterator);
		m_write_waiters.erase(iterator);
		deliver_write_waiter(std::move(waiter),
			asio::error::operation_aborted, false);
		return;
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::deliver_send_completion(
	const std::shared_ptr<send_operation> &operation,
	error_code error) noexcept
{
	if(not operation->completion)
		return;
	auto slot = asio::get_associated_cancellation_slot(operation->completion);
	if(slot.is_connected())
		slot.clear();
	try
	{
		auto completion = std::move(operation->completion);
		std::move(completion)(error, operation->transferred);
	}
	catch(...)
	{
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::complete_send_operation(
	const std::shared_ptr<send_operation> &operation,
	error_code error) noexcept
{
	if(operation->kind == send_kind::data)
	{
		m_completed_write_sequence = std::max(
			m_completed_write_sequence, operation->sequence);
		remember_write_error(operation->sequence, error);
	}
	deliver_send_completion(operation, error);
	if(operation->kind == send_kind::data)
		complete_write_waiters();
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::install_send_cancellation(
	const std::shared_ptr<send_operation> &operation)
{
	auto slot = asio::get_associated_cancellation_slot(operation->completion);
	if(not slot.is_connected())
		return;
	slot.assign([weak = this->weak_from_this(), id = operation->id](asio::cancellation_type type) noexcept
		{
		if( type == asio::cancellation_type::none )
		return;
		if( auto self = weak.lock() )
		{
			try
			{
				asio::dispatch(self->m_exec, [self, id] {
					self->cancel_queued_send(id);
				});
			}
			catch(...)
			{
			}
		} });
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel_queued_send(uint64_t id) noexcept
{
	for(auto &operation : m_data_write_queue)
	{
		if(operation->id != id or operation->cancel_requested)
			continue;
		operation->cancel_requested = true;
		// Keep a bounded tombstone in sequence order until earlier data writes
		// finish. This lets wait_written() preserve snapshot ordering without
		// allowing cancellation churn to bypass the queue limits.
		deliver_send_completion(operation, asio::error::operation_aborted);
		return;
	}

	for(auto iterator = m_control_write_queue.begin();
		iterator != m_control_write_queue.end(); ++iterator)
	{
		if((*iterator)->id != id)
			continue;
		auto operation = std::move(*iterator);
		m_control_write_queue.erase(iterator);
		if(operation->queued_counted)
		{
			m_queued_write_operations--;
			operation->queued_counted = false;
		}
		deliver_send_completion(operation, asio::error::operation_aborted);
		return;
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::fail_queued_controls(error_code error)
{
	while(not m_control_write_queue.empty())
	{
		auto operation = std::move(m_control_write_queue.front());
		m_control_write_queue.pop_front();
		if(operation->queued_counted)
			m_queued_write_operations--;
		complete_send_operation(operation, error);
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::fail_queued_writes(error_code error)
{
	fail_queued_controls(error);
	while(not m_data_write_queue.empty())
	{
		auto operation = std::move(m_data_write_queue.front());
		m_data_write_queue.pop_front();
		if(operation->queued_counted)
		{
			m_queued_write_operations--;
			m_queued_write_bytes -= operation->queued_payload_size;
		}
		complete_send_operation(operation, operation->cancel_requested ? error_code(asio::error::operation_aborted) : error);
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::start_wire_frame(prepared_frame frame,
	wire_frame_kind kind, std::shared_ptr<send_operation> operation) noexcept
{
	m_wire_write_active = true;
	auto self = this->shared_from_this();
	auto wire = const_buffer(frame.wire->data(), frame.wire->size());
	try
	{
		m_connection->write(wire,
			asio::any_completion_handler<void(error_code, size_t)>(
				[self, frame, kind, operation](error_code error, size_t wire_size) mutable
				{
					self->m_wire_write_active = false;
					const bool shutdown = self->m_state == connection_state::closed;
					const auto payload_size = wire_size > frame.header_size ? std::min(frame.payload_size, wire_size - frame.header_size) : 0;
					if(operation)
						operation->transferred += payload_size;
					if(not error and wire_size != frame.wire->size())
						error = make_error_code(std::errc::io_error);

					if(shutdown and not error)
						error = asio::error::operation_aborted;
					if(error)
					{
						const auto completion_error =
							self->m_protocol_failure_active and self->m_error ? self->m_error : error;
						if(operation)
						{
							if(operation->kind == send_kind::data)
								self->m_current_data.reset();
							self->complete_send_operation(operation, completion_error);
						}
						if(self->m_current_data)
						{
							auto data = std::exchange(self->m_current_data, {});
							self->complete_send_operation(data, completion_error);
						}
						self->fail_queued_writes(completion_error);
						self->m_pending_auto_pong.reset();
						self->m_pending_close_response.reset();
						if(not shutdown)
							self->fail(error);
						return;
					}

					switch(kind)
					{
					case wire_frame_kind::data:
						self->m_last_wire_was_control = false;
						operation->frame_index++;
						if(self->m_protocol_failure_active)
						{
							self->m_current_data.reset();
							self->complete_send_operation(operation,
								operation->frame_index == operation->frames.size() ? error_code{} : self->m_error);
							self->schedule_send();
							return;
						}
						if(operation->frame_index == operation->frames.size())
						{
							self->m_current_data.reset();
							self->complete_send_operation(operation, {});
						}
						break;
					case wire_frame_kind::application_control:
						self->m_last_wire_was_control = true;
						self->complete_send_operation(operation, {});
						break;
					case wire_frame_kind::automatic_pong:
						self->m_last_wire_was_control = true;
						break;
					case wire_frame_kind::local_close:
						self->m_last_wire_was_control = true;
						self->m_local_close_sent = true;
						if(self->m_peer_close)
							self->finish_close({}, true);
						else
							self->start_close_receive();
						return;
					case wire_frame_kind::close_response:
						self->m_last_wire_was_control = true;
						self->finish_close({}, true);
						return;
					case wire_frame_kind::protocol_close:
						self->m_last_wire_was_control = true;
						self->finish_protocol_failure();
						return;
					}
					self->schedule_send();
				}));
	}
	catch(...)
	{
		m_wire_write_active = false;
		auto error = exception_error(std::current_exception());
		if(operation)
		{
			if(operation->kind == send_kind::data)
				m_current_data.reset();
			complete_send_operation(operation, error);
		}
		if(m_current_data)
		{
			auto data = std::exchange(m_current_data, {});
			complete_send_operation(data, error);
		}
		fail_queued_writes(error);
		fail(error);
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::schedule_send()
{
	if(m_wire_write_active or not m_connection or m_transport_closed or
		(m_state == connection_state::failed and
			not m_protocol_failure_active) or
		m_state == connection_state::closed)
		return;

	if(m_pending_protocol_close and not m_current_data)
	{
		auto frame = std::exchange(
			m_pending_protocol_close, std::nullopt)
						 .value();
		start_wire_frame(std::move(frame), wire_frame_kind::protocol_close);
		return;
	}

	if(m_pending_close_response and not m_current_data)
	{
		auto payload = std::exchange(m_pending_close_response, std::nullopt);
		m_pending_auto_pong.reset();
		auto frame = prepare_control_frame(opcode::close,
			const_buffer(payload->data(), payload->size()));
		if(not frame)
		{
			fail_queued_writes(frame.error());
			fail(frame.error());
			return;
		}
		start_wire_frame(std::move(*frame), wire_frame_kind::close_response);
		return;
	}

	if(m_pending_local_close and not m_current_data and
		m_data_write_queue.empty())
	{
		auto frame = std::exchange(m_pending_local_close, std::nullopt).value();
		m_pending_auto_pong.reset();
		fail_queued_controls(make_error_code(errc::closing));
		start_wire_frame(std::move(frame), wire_frame_kind::local_close);
		return;
	}

	const bool data_available = m_current_data or not m_data_write_queue.empty();
	if((not m_last_wire_was_control or not data_available) and
		m_pending_auto_pong and not m_pending_local_close and
		not m_pending_close_response)
	{
		auto payload = std::exchange(m_pending_auto_pong, std::nullopt);
		auto frame = prepare_control_frame(opcode::pong,
			const_buffer(payload->data(), payload->size()));
		if(not frame)
		{
			fail_queued_writes(frame.error());
			fail(frame.error());
			return;
		}
		start_wire_frame(std::move(*frame), wire_frame_kind::automatic_pong);
		return;
	}

	if((not m_last_wire_was_control or not data_available) and
		not m_control_write_queue.empty() and not m_pending_local_close and
		not m_pending_close_response)
	{
		auto operation = std::move(m_control_write_queue.front());
		m_control_write_queue.pop_front();
		if(operation->queued_counted)
		{
			m_queued_write_operations--;
			operation->queued_counted = false;
		}
		auto frame = operation->frames.front();
		start_wire_frame(std::move(frame),
			wire_frame_kind::application_control, std::move(operation));
		return;
	}

	if(data_available)
	{
		if(not m_current_data)
		{
			while(not m_data_write_queue.empty() and
				m_data_write_queue.front()->cancel_requested)
			{
				auto cancelled = std::move(m_data_write_queue.front());
				m_data_write_queue.pop_front();
				if(cancelled->queued_counted)
				{
					m_queued_write_operations--;
					m_queued_write_bytes -= cancelled->queued_payload_size;
					cancelled->queued_counted = false;
				}
				complete_send_operation(cancelled,
					asio::error::operation_aborted);
			}
			if(m_data_write_queue.empty())
			{
				schedule_send();
				return;
			}
			m_current_data = std::move(m_data_write_queue.front());
			m_data_write_queue.pop_front();
			if(m_current_data->queued_counted)
			{
				m_queued_write_operations--;
				m_queued_write_bytes -= m_current_data->queued_payload_size;
				m_current_data->queued_counted = false;
			}
		}
		start_wire_frame(m_current_data->frames[m_current_data->frame_index],
			wire_frame_kind::data, m_current_data);
		return;
	}

	// With no data pending, fairness no longer limits consecutive controls.
	m_last_wire_was_control = false;
	if(m_pending_auto_pong or m_pending_local_close or
		not m_control_write_queue.empty())
		schedule_send();
}

template <core_concepts::exec Exec>
error_code stream_impl<Exec>::enqueue_send_operation(
	std::shared_ptr<send_operation> operation) noexcept
{
	if(not send_engine_busy())
	{
		if(operation->kind == send_kind::data)
			m_current_data = std::move(operation);
		else
		{
			auto frame = operation->frames.front();
			start_wire_frame(std::move(frame),
				wire_frame_kind::application_control, std::move(operation));
			return {};
		}
		schedule_send();
		return {};
	}

	if(not queue_has_capacity(operation->kind,
		   operation->queued_payload_size))
		return make_error_code(errc::write_queue_full);
	try
	{
		m_queued_write_operations++;
		operation->queued_counted = true;
		if(operation->kind == send_kind::data)
		{
			m_queued_write_bytes += operation->queued_payload_size;
			m_data_write_queue.push_back(operation);
		}
		else
			m_control_write_queue.push_back(operation);
		return {};
	}
	catch(const std::bad_alloc &)
	{
		m_queued_write_operations--;
		operation->queued_counted = false;
		if(operation and operation->kind == send_kind::data)
			m_queued_write_bytes -= operation->queued_payload_size;
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		m_queued_write_operations--;
		operation->queued_counted = false;
		if(operation and operation->kind == send_kind::data)
			m_queued_write_bytes -= operation->queued_payload_size;
		return make_error_code(std::errc::io_error);
	}
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_write_message(message_type type,
	std::span<const const_buffer> buffers, Handler &&handler)
{
	auto completion = asio::any_completion_handler<void(error_code, size_t)>(
		std::forward<Handler>(handler));
	auto self = this->shared_from_this();
	if(self->m_state != connection_state::open)
	{
		auto error = self->state_write_error();
		asio::post(self->m_exec, [handler = std::move(completion), error]() mutable
			{ std::move(handler)(error, 0); });
		return;
	}

	auto frames = self->prepare_frames(type, buffers);
	if(not frames)
	{
		auto error = frames.error();
		asio::post(self->m_exec, [handler = std::move(completion), error]() mutable
			{ std::move(handler)(error, 0); });
		return;
	}

	std::shared_ptr<send_operation> operation;
	try
	{
		operation = std::make_shared<send_operation>();
		operation->kind = send_kind::data;
		operation->frames = std::move(*frames);
		operation->completion = std::move(completion);
		operation->id = ++self->m_next_send_operation_id;
		for(const auto &frame : operation->frames)
			operation->queued_payload_size += frame.payload_size;
		self->install_send_cancellation(operation);
		operation->sequence = ++self->m_last_write_sequence;
		if(auto error = self->enqueue_send_operation(operation))
		{
			self->m_last_write_sequence--;
			self->deliver_send_completion(operation, error);
		}
	}
	catch(const std::bad_alloc &)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if(operation and operation->completion)
			self->deliver_send_completion(operation, error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, 0);
				});
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if(operation and operation->completion)
			self->deliver_send_completion(operation, error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, 0);
				});
	}
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_write_control(opcode op, const const_buffer &payload,
	Handler &&handler)
{
	auto completion = asio::any_completion_handler<void(error_code, size_t)>(
		std::forward<Handler>(handler));
	auto self = this->shared_from_this();
	if(self->m_state != connection_state::open)
	{
		auto error = self->state_write_error();
		asio::post(self->m_exec, [handler = std::move(completion), error]() mutable
			{ std::move(handler)(error, 0); });
		return;
	}
	auto frame = self->prepare_control_frame(op, payload);
	if(not frame)
	{
		auto error = frame.error();
		asio::post(self->m_exec, [handler = std::move(completion), error]() mutable
			{ std::move(handler)(error, 0); });
		return;
	}

	std::shared_ptr<send_operation> operation;
	try
	{
		operation = std::make_shared<send_operation>();
		operation->kind = send_kind::application_control;
		operation->frames.push_back(std::move(*frame));
		operation->completion = std::move(completion);
		operation->id = ++self->m_next_send_operation_id;
		operation->queued_payload_size = operation->frames.front().payload_size;
		self->install_send_cancellation(operation);
		if(auto error = self->enqueue_send_operation(operation))
			self->deliver_send_completion(operation, error);
	}
	catch(const std::bad_alloc &)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if(operation and operation->completion)
			self->deliver_send_completion(operation, error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, 0);
				});
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if(operation and operation->completion)
			self->deliver_send_completion(operation, error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, 0);
				});
	}
}

template <core_concepts::exec Exec>
size_t stream_impl<Exec>::write_control(opcode op, const const_buffer &payload,
	error_code &error) noexcept
{
	error.clear();
	if(m_state != connection_state::open)
	{
		error = state_write_error();
		return 0;
	}
	auto frame = prepare_control_frame(op, payload);
	if(not frame)
	{
		error = frame.error();
		return 0;
	}
	auto transferred = write_prepared(*frame, error);
	if(error)
		fail(error);
	return transferred;
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::wait_written(error_code &error) noexcept
{
	if(m_completed_write_sequence < m_last_write_sequence)
	{
		error = make_error_code(std::errc::operation_in_progress);
		return;
	}
	error = observe_write_error(m_last_write_sequence);
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_wait_written(Handler &&handler)
{
	auto completion = asio::any_completion_handler<void(error_code)>(
		std::forward<Handler>(handler));
	const auto target = m_last_write_sequence;
	if(target <= m_completed_write_sequence)
	{
		auto error = observe_write_error(target);
		asio::post(m_exec, [handler = std::move(completion), error]() mutable
			{ std::move(handler)(error); });
		return;
	}
	std::shared_ptr<write_waiter> waiter;
	bool queued = false;
	try
	{
		auto associated_allocator =
			asio::get_associated_allocator(completion);
		using waiter_allocator_t = std::allocator_traits<
			decltype(associated_allocator)>::template rebind_alloc<write_waiter>;
		waiter = std::allocate_shared<write_waiter>(
			waiter_allocator_t(associated_allocator));
		waiter->id = ++m_next_write_waiter_id;
		waiter->target = target;
		waiter->completion = std::move(completion);
		m_write_waiters.push_back(waiter);
		queued = true;

		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
		{
			slot.assign([weak = this->weak_from_this(), id = waiter->id](asio::cancellation_type type) noexcept
				{
				if( type == asio::cancellation_type::none )
				return;
				if( auto self = weak.lock() )
				{
					try
					{
						asio::dispatch(self->m_exec, [self, id] {
							self->cancel_write_waiter(id);
						});
					}
					catch(...)
					{
					}
				} });
		}
	}
	catch(const std::bad_alloc &)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if(waiter)
		{
			if(queued)
			{
				auto iterator = std::ranges::find(m_write_waiters, waiter);
				if(iterator != m_write_waiters.end())
					m_write_waiters.erase(iterator);
			}
			deliver_write_waiter(waiter, error);
		}
		else
			asio::post(m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error);
				});
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if(waiter)
		{
			if(queued)
			{
				auto iterator = std::ranges::find(m_write_waiters, waiter);
				if(iterator != m_write_waiters.end())
					m_write_waiters.erase(iterator);
			}
			deliver_write_waiter(waiter, error);
		}
		else
			asio::post(m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error);
				});
	}
}

template <core_concepts::exec Exec>
sys_expected<> stream_impl<Exec>::retain_protocol_payload(
	std::optional<std::vector<std::byte>> &slot,
	const std::vector<std::byte> &payload) noexcept
{
	try
	{
		slot = payload;
		return make_sys_expected();
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

template <core_concepts::exec Exec>
sys_expected<> stream_impl<Exec>::queue_automatic_pong(
	const std::vector<std::byte> &payload) noexcept
{
	auto retained = retain_protocol_payload(m_pending_auto_pong, payload);
	if(retained)
		schedule_send();
	return retained;
}

template <core_concepts::exec Exec>
sys_expected<> stream_impl<Exec>::begin_peer_close(
	const std::vector<std::byte> &payload) noexcept
{
	auto remembered = remember_peer_close(payload);
	if(not remembered)
		return remembered;
	start_close_deadline();
	if(m_state == connection_state::failed)
		return sys_unexpected(m_error);
	if(m_config.close_timeout <= std::chrono::milliseconds::zero())
	{
		m_pending_auto_pong.reset();
		fail_queued_writes(make_error_code(std::errc::broken_pipe));
		return make_sys_expected();
	}
	if(m_local_close_sent)
	{
		finish_close({}, true);
		return make_sys_expected();
	}
	if(not m_local_close_initiated)
	{
		auto retained = retain_protocol_payload(m_pending_close_response, payload);
		if(not retained)
			return retained;
	}
	m_pending_auto_pong.reset();
	fail_queued_writes(make_error_code(std::errc::broken_pipe));
	if(m_state == connection_state::closing)
		schedule_send();
	return make_sys_expected();
}

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_IPP
