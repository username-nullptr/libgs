// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_IPP

namespace libgs::websocket::detail
{

template <core_concepts::exec Exec>
auto stream_impl<Exec>::retained_close_info(bool clean) const -> close_info_t
{
	auto result = m_peer_close.value_or(close_info_t{});
	result.clean = clean;
	return result;
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::complete_close_waiters(error_code error) noexcept
{
	while( not m_close_waiters.empty() )
	{
		auto waiter = std::move(m_close_waiters.front());
		m_close_waiters.pop_front();

		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if( slot.is_connected() )
			slot.clear();
		try {
			auto completion = std::move(waiter->completion);
			std::move(completion)(error,
				m_close_result.value_or(retained_close_info())
			);
		}
		catch(...) {}
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel_close_waiter(uint64_t id) noexcept
{
	for(auto iterator = m_close_waiters.begin();
		iterator != m_close_waiters.end(); ++iterator)
	{
		if( (*iterator)->id != id )
			continue;

		auto waiter = std::move(*iterator);
		m_close_waiters.erase(iterator);
		try {
			auto completion = std::move(waiter->completion);
			std::move(completion)(asio::error::operation_aborted, close_info_t{});
		}
		catch(...) {}
		return ;
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::start_close_deadline() noexcept
{
	if( m_close_timer or (m_state != connection_state::closing and not m_protocol_failure_active) )
		return ;
	try {
		m_close_timer = std::make_shared<asio::steady_timer>(m_exec);
		m_close_timer->expires_after (
			std::chrono::duration_cast<asio::steady_timer::duration>
				(m_config.close_timeout)
		);
		auto timer = m_close_timer;
		auto self = this->shared_from_this();

		timer->async_wait([self, timer](error_code error)
		{
			if( error or self->m_close_timer != timer )
				return ;

			if( self->m_protocol_failure_active )
			{
				self->finish_protocol_failure(true);
				return ;
			}
			if( self->m_state != connection_state::closing )
				return ;

			self->finish_close(asio::error::timed_out, false, true);
		});
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		fail(error);
		complete_close_waiters(error);
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::finish_close(error_code error, bool clean, bool cancel_transport) noexcept
{
	if( m_state == connection_state::closed or m_state == connection_state::failed )
		return ;

	if( m_close_timer )
	{
		try {
			ignore_unused(m_close_timer->cancel());
		}
		catch(...) {}
		m_close_timer.reset();
	}
	if( cancel_transport and m_connection and not m_transport_closed )
		ignore_unused(m_connection->cancel());

	error_code close_error;
	close_transport(close_error);

	if( not error and close_error )
	{
		fail(close_error);
		complete_close_waiters(close_error);
		return ;
	}
	m_pending_auto_pong.reset();
	m_pending_local_close.reset();
	m_pending_close_response.reset();

	if( m_peer_close )
		m_peer_close->clean = clean and not error and not close_error;

	m_close_result = retained_close_info(clean and not error and not close_error);
	m_state = connection_state::closed;

	stop_control_observer(error ? error : error_code(asio::error::eof));
	fail_queued_writes(error ? error : make_error_code(std::errc::broken_pipe));
	complete_close_waiters(error);
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::begin_local_close(prepared_frame frame) noexcept
{
	m_local_close_initiated = true;
	m_state = connection_state::closing;
	m_pending_auto_pong.reset();

	stop_control_observer(make_error_code(errc::closing));
	fail_queued_controls(make_error_code(errc::closing));

	if( m_config.close_timeout > std::chrono::milliseconds::zero() )
		m_pending_local_close = std::move(frame);

	start_close_deadline();
	if( m_state == connection_state::closing and m_pending_local_close )
		schedule_send();
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::start_close_receive() noexcept
{
	if( m_read_active or m_state != connection_state::closing or
		not m_local_close_sent or m_peer_close )
		return ;

	m_read_active = true;
	auto self = this->shared_from_this();
	try {
		asio::co_spawn(self->m_exec, [self]() -> awaitable<error_code>
		{
			for(;;)
			{
				if( self->m_state != connection_state::closing )
					co_return asio::error::operation_aborted;

				while( self->available_read_data().size() != 0 )
				{
					auto event = self->consume_frame_data();
					if( not event )
						co_return event.error();

					if( not *event )
						continue;

					auto &value = **event;
					if( value.data )
						continue;

					if( value.op == opcode::ping or value.op == opcode::pong )
					{
						if( value.op == opcode::ping and self->m_config.automatic_pong )
						{
							auto queued = self->queue_automatic_pong(value.control);
							if( not queued )
								co_return queued.error();
						}
						self->remember_control(value.op, std::move(value.control));
						continue;
					}
					if( value.op == opcode::close )
					{
						auto remembered = self->remember_peer_close(value.control);
						co_return remembered ? error_code {} : remembered.error();
					}
				}
				if( self->m_read_error )
					co_return std::exchange(self->m_read_error, {});

				auto owner = self->m_read_buffer;
				auto [error, size] = co_await http::detail::initiate_connection_io (
				[self, owner](auto next_handler) mutable
				{
					self->m_connection->read (
						mutable_buffer(owner->data(), owner->size()),
						asio::any_completion_handler<void(error_code,size_t)>
							(std::move(next_handler))
					);
				},
				asio::as_tuple(asio::use_awaitable));

				if( size > owner->size() )
					co_return make_error_code(std::errc::io_error);

				self->m_read_size = size;
				self->m_read_offset = 0;
				self->m_read_error = error;

				if( size == 0 )
				{
					if( not error )
						error = make_error_code(std::errc::io_error);
					self->m_read_error.clear();
					co_return error;
				}
			}
		},
		asio::bind_executor(self->m_exec,
		[self](const std::exception_ptr &exception, error_code error)
		{
			self->m_read_active = false;
			if( self->m_state != connection_state::closing )
				return ;

			if( auto exception_error_code = exception_error(exception) )
				error = exception_error_code;

			if( error )
			{
				if( error == asio::error::eof )
					self->finish_close(error, false);

				else if( error != asio::error::operation_aborted )
				{
					self->fail(error);
					self->complete_close_waiters(error);
				}
				return ;
			}
			self->finish_close({}, true);
		}));
	}
	catch(...)
	{
		m_read_active = false;
		auto error = exception_error(std::current_exception());
		fail(error);
		complete_close_waiters(error);
	}
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::close(const close_frame &frame, error_code &error) noexcept -> close_info_t
{
	error.clear();
	if( m_state == connection_state::closed )
		return m_close_result.value_or(retained_close_info());

	if( m_state == connection_state::failed )
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return retained_close_info();
	}
	if( m_state == connection_state::idle )
	{
		error = make_error_code(errc::not_open);
		return {};
	}
	if( m_state == connection_state::closing or m_read_active or send_engine_busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return retained_close_info();
	}
	auto payload = encode_close_payload(frame);
	if( not payload )
	{
		error = payload.error();
		return {};
	}
	auto prepared = prepare_control_frame(opcode::close, payload->buffer());
	if( not prepared )
	{
		error = prepared.error();
		return {};
	}
	m_local_close_initiated = true;
	m_state = connection_state::closing;
	stop_control_observer(make_error_code(errc::closing));

	const auto started = std::chrono::steady_clock::now();
	auto expired = [&]
	{
		return m_config.close_timeout <= std::chrono::milliseconds::zero() or
			std::chrono::steady_clock::now() - started >= m_config.close_timeout;
	};
	if( expired() )
	{
		error = asio::error::timed_out;
		finish_close(error, false, true);
		return *m_close_result;
	}
	ignore_unused(write_prepared(*prepared, error));
	if( error )
	{
		fail(error);
		return retained_close_info();
	}
	m_local_close_sent = true;
	for(;;)
	{
		if( expired() )
		{
			error = asio::error::timed_out;
			finish_close(error, false, true);
			return *m_close_result;
		}
		while(available_read_data().size() != 0)
		{
			auto event = consume_frame_data();
			if( not event )
			{
				error = event.error();
				fail(error);
				return retained_close_info();
			}
			if( not *event )
				continue;

			auto &value = **event;
			if( value.data )
				continue;

			if( value.op == opcode::ping or value.op == opcode::pong )
			{
				remember_control(value.op, std::move(value.control));
				continue;
			}
			if( value.op == opcode::close )
			{
				auto remembered = remember_peer_close(value.control);
				if( not remembered )
				{
					error = remembered.error();
					fail(error);
					return retained_close_info();
				}
				finish_close({}, true);
				error.clear();
				return *m_close_result;
			}
		}
		error_code read_error;
		auto size = m_connection->read (
			mutable_buffer(m_read_buffer->data(), m_read_buffer->size()),
			read_error
		);
		if( size > m_read_buffer->size() )
		{
			error = make_error_code(std::errc::io_error);
			fail(error);
			return retained_close_info();
		}
		m_read_size = size;
		m_read_offset = 0;
		m_read_error = read_error;

		if( size == 0 )
		{
			error = read_error ? read_error : make_error_code(std::errc::io_error);
			m_read_error.clear();

			if( error == asio::error::eof )
				finish_close(error, false);
			else
				fail(error);

			return m_close_result.value_or(retained_close_info());
		}
	}
	return retained_close_info();
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::wait_closed(error_code &error) noexcept -> close_info_t
{
	if( m_state == connection_state::closed )
	{
		error.clear();
		return m_close_result.value_or(retained_close_info());
	}
	if( m_state == connection_state::failed )
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return retained_close_info();
	}
	if( m_state == connection_state::idle )
		error = make_error_code(errc::not_open);
	else
		error = make_error_code(std::errc::operation_would_block);

	return retained_close_info();
}

template <core_concepts::exec Exec>
template <typename Handler>
bool stream_impl<Exec>::add_close_waiter(Handler &&handler) noexcept
{
	auto completion = asio::any_completion_handler
		<void(error_code, close_info_t)>(std::forward<Handler>(handler));

	std::shared_ptr<close_wait_operation> waiter;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);
		using allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::rebind_alloc<close_wait_operation>;

		waiter = std::allocate_shared<close_wait_operation>(
			allocator_t(associated_allocator)
		);
		waiter->id = ++m_next_close_waiter_id;
		waiter->completion = std::move(completion);

		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if( slot.is_connected() )
		{
			slot.assign([weak = this->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto self = weak.lock() )
				{
					try {
						asio::dispatch(self->m_exec, [self, id] {
							self->cancel_close_waiter(id);
						});
					}
					catch(...) {}
				}
			});
		}
		m_close_waiters.push_back(waiter);
		return true;
	}
	catch(...)
	{
		auto error = exception_error(std::current_exception());
		if( waiter and waiter->completion )
			completion = std::move(waiter->completion);
		try {
			std::move(completion)(error, close_info_t{});
		}
		catch(...) {}
	}
	return false;
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_close(close_frame frame, Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code, close_info_t)>(std::forward<Handler>(handler));

	if( m_state == connection_state::closed )
	{
		auto result = m_close_result.value_or(retained_close_info());
		asio::post(m_exec, [handler = std::move(completion), result]() mutable {
			std::move(handler)(error_code{}, result);
		});
		return ;
	}
	if( m_state == connection_state::failed or m_state == connection_state::idle )
	{
		auto error = m_state == connection_state::failed ?
			(m_error ? m_error : make_error_code(std::errc::io_error)) :
			make_error_code(errc::not_open);

		asio::post(m_exec, [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, close_info_t{});
		});
		return ;
	}
	if( m_state == connection_state::closing )
	{
		ignore_unused(add_close_waiter(std::move(completion)));
		return ;
	}
	auto payload = encode_close_payload(frame);
	if( not payload )
	{
		auto error = payload.error();
		asio::post(m_exec, [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, close_info_t{});
		});
		return ;
	}
	auto prepared = prepare_control_frame(opcode::close, payload->buffer());
	if( not prepared )
	{
		auto error = prepared.error();
		asio::post(m_exec, [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, close_info_t{});
		});
		return ;
	}
	if( not add_close_waiter(std::move(completion)) )
		return ;

	begin_local_close(std::move(*prepared));
	complete_read_waiter(make_error_code(errc::closing));
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_wait_closed(Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code, close_info_t)>(std::forward<Handler>(handler));

	if( m_state == connection_state::closed )
	{
		auto result = m_close_result.value_or(retained_close_info());
		asio::post(m_exec, [handler = std::move(completion), result]() mutable {
			std::move(handler)(error_code{}, result);
		});
		return ;
	}
	if( m_state == connection_state::failed or m_state == connection_state::idle )
	{
		auto error = m_state == connection_state::failed ?
			(m_error ? m_error : make_error_code(std::errc::io_error)) :
			make_error_code(errc::not_open);

		asio::post(m_exec, [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, close_info_t{});
		});
		return ;
	}
	ignore_unused(add_close_waiter(std::move(completion)));
}

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_IPP
