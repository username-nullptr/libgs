// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CLOSE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CLOSE_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
# error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H

namespace libgs::websocket
{

template <core_concepts::exec Exec>
bool basic_stream<Exec>::impl::local_close_started() const noexcept
{
	return m_local_close_phase != local_close_phase::none;
}

template <core_concepts::exec Exec>
bool basic_stream<Exec>::impl::local_close_sent() const noexcept
{
	return m_local_close_phase == local_close_phase::sent;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::mark_local_close_queued() noexcept
{
	if( m_local_close_phase == local_close_phase::none )
		m_local_close_phase = local_close_phase::queued;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::mark_local_close_sent() noexcept
{
	m_local_close_phase = local_close_phase::sent;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::handle_wire_frame_sent
(detail::wire_frame_kind kind) noexcept
{
	switch(kind)
	{
	case detail::wire_frame_kind::local_close:
		mark_local_close_sent();
		if( m_peer_close )
			finish_close({}, true);
		else
			start_close_receive();
		break;

	case detail::wire_frame_kind::close_response:
		finish_close({}, true);
		break;

	case detail::wire_frame_kind::protocol_close:
		finish_protocol_failure();
		break;

	case detail::wire_frame_kind::data:
	case detail::wire_frame_kind::application_control:
	case detail::wire_frame_kind::auto_pong:
		break;
	}
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::remember_peer_close(const std::vector<std::byte> &payload) noexcept
{
	auto decoded = decode_close_payload (
		const_buffer(payload.data(), payload.size())
	);
	if( not decoded )
		return sys_unexpected(decoded.error());
	try {
		m_peer_close = close_info_t {
			.code = decoded->code,
			.reason = std::string(decoded->reason),
			.clean = false,
		};
		m_state = connection_state::closing;
		stop_automatic_ping();
		return make_sys_expected();
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::begin_peer_close(const std::vector<std::byte> &payload) noexcept
{
	if( auto remembered = remember_peer_close(payload); not remembered )
		return remembered;

	start_close_deadline();
	if( m_state == connection_state::failed )
		return sys_unexpected(m_error);

	if( m_config.close_timeout <= std::chrono::milliseconds::zero() )
	{
		m_send_engine.clear_auto_pong();
		m_send_engine.fail_queued_writes(make_error_code(std::errc::broken_pipe));
		return make_sys_expected();
	}
	if( local_close_sent() )
	{
		finish_close({}, true);
		return make_sys_expected();
	}
	if( not local_close_started() )
	{
		auto retained = m_send_engine.queue_close_response(payload);
		if( not retained )
			return retained;
	}
	m_send_engine.clear_auto_pong();
	m_send_engine.fail_queued_writes(make_error_code(std::errc::broken_pipe));

	if( m_state == connection_state::closing )
		m_send_engine.schedule();
	return make_sys_expected();
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::handle_sync_peer_close
(const std::vector<std::byte> &payload) noexcept
{
	if( auto remembered = remember_peer_close(payload); not remembered )
		return remembered;

	if( m_send_engine.busy() )
	{
		start_close_deadline();
		if( m_state == connection_state::failed )
			return sys_unexpected(m_error);

		auto retained = m_send_engine.queue_close_response(payload);
		if( not retained )
			return retained;

		m_send_engine.clear_auto_pong();
		m_send_engine.fail_queued_writes(make_error_code(std::errc::broken_pipe));
		m_send_engine.schedule();
		return make_sys_expected();
	}

	auto frame = m_send_engine.prepare_control(opcode::close,
		const_buffer(payload.data(), payload.size())
	);
	if( not frame )
		return sys_unexpected(frame.error());

	error_code error;
	ignore_unused(write_prepared(*frame, error));
	if( error )
		return sys_unexpected(error);

	finish_close({}, true);
	return make_sys_expected();
}

template <core_concepts::exec Exec>
auto basic_stream<Exec>::impl::retained_close_info(bool clean) const -> close_info_t
{
	auto result = m_peer_close.value_or(close_info_t{});
	result.clean = clean;
	return result;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_closed(closed_callback_t callback)
{
	if( m_closed_notified )
		return ;

	m_on_closed = std::move(callback);
	if( m_state == connection_state::closed or m_state == connection_state::failed )
		notify_closed();
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::notify_closed() noexcept
{
	if( m_closed_notified or not m_on_closed )
		return ;

	m_closed_notified = true;
	auto callback = std::move(m_on_closed);
	auto result = m_close_result.value_or(retained_close_info());
	try {
		callback(result);
	}
	catch(...) {}
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::complete_close_waiters(error_code error) noexcept
{
	notify_closed();
	while( not m_close_waiters.empty() )
	{
		auto waiter = std::move(m_close_waiters.front());
		m_close_waiters.pop_front();

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
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
void basic_stream<Exec>::impl::cancel_close_waiter(uint64_t id) noexcept
{
	for(auto it=m_close_waiters.begin();
		it!=m_close_waiters.end(); ++it)
	{
		if( (*it)->id != id )
			continue;

		auto waiter = std::move(*it);
		m_close_waiters.erase(it);
		try {
			auto completion = std::move(waiter->completion);
			std::move(completion)(asio::error::operation_aborted, close_info_t{});
		}
		catch(...) {}
		return ;
	}
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::start_close_deadline() noexcept
{
	if( m_close_timer or
		(m_state != connection_state::closing and not m_protocol_failure_active) )
		return ;
	try {
		m_close_timer = std::make_shared<asio::steady_timer>(m_exec);
		m_close_timer->expires_after (
			std::chrono::duration_cast<asio::steady_timer::duration>(m_config.close_timeout)
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
void basic_stream<Exec>::impl::finish_close(error_code error, bool clean, bool cancel_transport) noexcept
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
	m_send_engine.clear_auto_pong();
	m_send_engine.clear_local_close();
	m_send_engine.clear_close_response();

	if( m_peer_close )
		m_peer_close->clean = clean and not error and not close_error;

	m_close_result = retained_close_info(clean and not error and not close_error);
	m_state = connection_state::closed;

	stop_automatic_ping();
	m_send_engine.fail_queued_writes (
		 error ? error : make_error_code(std::errc::broken_pipe)
	);
	complete_close_waiters(error);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::begin_local_close(prepared_frame frame) noexcept
{
	mark_local_close_queued();
	m_state = connection_state::closing;

	stop_automatic_ping();
	m_send_engine.clear_auto_pong();
	m_send_engine.fail_queued_controls(make_error_code(errc::closing));

	if( m_config.close_timeout > std::chrono::milliseconds::zero() )
		m_send_engine.queue_local_close(std::move(frame));

	start_close_deadline();
	if( m_state == connection_state::closing and m_send_engine.has_local_close() )
		m_send_engine.schedule();
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::start_close_receive() noexcept
{
	if( m_receive_engine.active() or m_state != connection_state::closing or
		not local_close_sent() or m_peer_close )
		return ;

	m_receive_engine.set_active(true);
	auto self = this->shared_from_this();
	try {
		asio::co_spawn(self->executor(), [self]() -> awaitable<error_code>
		{
			for(;;)
			{
				if( self->m_state != connection_state::closing )
					co_return asio::error::operation_aborted;

				auto next = co_await self->m_receive_engine.async_next_event();
				if( next.error )
					co_return next.error;

				auto &value = *next.event;
				if( value.data )
					continue;

				if( value.op == opcode::ping or value.op == opcode::pong )
				{
					auto handled = self->handle_async_control(value.op, value.control);
					if( not handled )
						co_return handled.error();
					continue;
				}
				if( value.op == opcode::close )
				{
					auto remembered = self->remember_peer_close(value.control);
					co_return remembered ? error_code {} : remembered.error();
				}
			}
		},
		asio::bind_executor(self->executor(),
		[self](const std::exception_ptr &exception, error_code error)
		{
			self->m_receive_engine.set_active(false);
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
		m_receive_engine.set_active(false);
		auto error = exception_error(std::current_exception());
		fail(error);
		complete_close_waiters(error);
	}
}

template <core_concepts::exec Exec>
auto basic_stream<Exec>::impl::close(const close_frame &frame, error_code &error) noexcept -> close_info_t
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
	if( m_state == connection_state::closing or m_receive_engine.active() or m_send_engine.busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return retained_close_info();
	}
	auto prepared = m_send_engine.prepare_close(frame);
	if( not prepared )
	{
		error = prepared.error();
		return {};
	}
	mark_local_close_queued();
	m_state = connection_state::closing;

	stop_automatic_ping();
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
	mark_local_close_sent();
	for(;;)
	{
		if( expired() )
		{
			error = asio::error::timed_out;
			finish_close(error, false, true);
			return *m_close_result;
		}
		auto next = m_receive_engine.next_event();
		if( next.error )
		{
			error = next.error;
			if( error == asio::error::eof )
				finish_close(error, false);
			else
				fail(error);

			return m_close_result.value_or(retained_close_info());
		}

		auto &value = *next.event;
		if( value.data )
			continue;

		if( value.op == opcode::ping or value.op == opcode::pong )
		{
			auto handled = handle_sync_control(value.op, value.control);
			if( not handled )
			{
				error = handled.error();
				fail(error);
				return retained_close_info();
			}
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
}

template <core_concepts::exec Exec>
auto basic_stream<Exec>::impl::wait_closed(error_code &error) noexcept -> close_info_t
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
bool basic_stream<Exec>::impl::add_close_waiter(Handler &&handler) noexcept
{
	auto completion = asio::any_completion_handler
		<void(error_code, close_info_t)>(std::forward<Handler>(handler));

	std::shared_ptr<close_wait_operation> waiter;
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);
		using allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::template rebind_alloc<close_wait_operation>;

		waiter = std::allocate_shared<close_wait_operation>(
			allocator_t(associated_allocator)
		);
		waiter->id = ++m_next_close_waiter_id;
		waiter->completion = std::move(completion);

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
		{
			slot.assign([weak = this->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto self = weak.lock() )
				{
					try {
						asio::dispatch(self->executor(), [self, id] {
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
void basic_stream<Exec>::impl::async_close(close_frame frame, Handler &&handler)
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
	auto prepared = m_send_engine.prepare_close(frame);
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
	m_receive_engine.complete_read_waiter(make_error_code(errc::closing));
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_wait_closed(Handler &&handler)
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

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CLOSE_IPP
