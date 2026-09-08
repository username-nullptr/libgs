// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_LIFECYCLE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_LIFECYCLE_IPP

// Cancellation, shutdown and terminal failure handling.

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel(error_code &error) noexcept
{
	if(not m_connection or m_transport_closed)
	{
		error.clear();
		return;
	}
	auto result = m_connection->cancel();
	error = result ? error_code{} : result.error();
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::shutdown(error_code &error) noexcept
{
	if(m_close_timer)
	{
		try
		{
			ignore_unused(m_close_timer->cancel());
		}
		catch(...)
		{
		}
		m_close_timer.reset();
	}
	if(m_state == connection_state::closed)
	{
		complete_read_waiter(asio::error::operation_aborted);
		stop_control_observer(asio::error::operation_aborted);
		complete_close_waiters(asio::error::operation_aborted);
		error.clear();
		return;
	}
	if(not m_connection or m_transport_closed)
	{
		m_close_result = retained_close_info(false);
		m_state = connection_state::closed;
		complete_read_waiter(asio::error::operation_aborted);
		stop_control_observer(asio::error::operation_aborted);
		complete_close_waiters(asio::error::operation_aborted);
		error.clear();
		return;
	}
	complete_read_waiter(asio::error::operation_aborted);
	stop_control_observer(asio::error::operation_aborted);
	fail_queued_writes(asio::error::operation_aborted);
	m_pending_auto_pong.reset();
	m_pending_local_close.reset();
	m_pending_close_response.reset();
	m_pending_protocol_close.reset();
	m_protocol_failure_active = false;
	ignore_unused(m_connection->cancel());
	auto result = m_connection->close();
	m_transport_closed = true;
	m_close_result = retained_close_info(false);
	m_state = connection_state::closed;
	complete_close_waiters(asio::error::operation_aborted);
	error = result ? error_code{} : result.error();
}

template <core_concepts::exec Exec>
std::optional<close_code>
stream_impl<Exec>::protocol_failure_code(error_code error) noexcept
{
	if(error.category() == protocol_error_category())
		return close_code_for(static_cast<protocol_errc>(error.value()));
	if(error == errc::message_too_big)
		return close_code::message_too_big;
	return std::nullopt;
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::finish_protocol_failure(bool cancel_transport) noexcept
{
	if(not m_protocol_failure_active)
		return;
	if(m_close_timer)
	{
		try
		{
			ignore_unused(m_close_timer->cancel());
		}
		catch(...)
		{
		}
		m_close_timer.reset();
	}
	if(cancel_transport and m_connection and not m_transport_closed)
		ignore_unused(m_connection->cancel());
	m_pending_protocol_close.reset();
	m_protocol_failure_active = false;
	error_code close_error;
	close_transport(close_error);
	m_close_result = retained_close_info(false);
	m_state = connection_state::failed;
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::begin_protocol_failure(
	error_code error, bool synchronous) noexcept
{
	auto code = protocol_failure_code(error);
	if(not code or m_state != connection_state::open or
		m_local_close_sent or m_transport_closed)
	{
		fail(error);
		return;
	}

	if(not m_error)
		m_error = error;
	m_state = connection_state::failed;
	m_protocol_failure_active = true;
	m_close_result = retained_close_info(false);
	complete_read_waiter(m_error);
	stop_control_observer(m_error);
	complete_close_waiters(m_error);
	fail_queued_writes(m_error);
	m_pending_auto_pong.reset();
	m_pending_local_close.reset();
	m_pending_close_response.reset();

	if(m_current_data and not m_wire_write_active)
	{
		auto operation = std::exchange(m_current_data, {});
		complete_send_operation(operation, m_error);
	}

	auto payload = encode_close_payload(close_frame(*code));
	if(not payload)
	{
		fail(payload.error());
		return;
	}
	auto prepared = prepare_control_frame(opcode::close, payload->buffer());
	if(not prepared)
	{
		fail(prepared.error());
		return;
	}

	if(m_config.close_timeout <= std::chrono::milliseconds::zero())
	{
		finish_protocol_failure(true);
		return;
	}
	if(synchronous and not m_wire_write_active and not m_current_data)
	{
		error_code write_error;
		ignore_unused(write_prepared(*prepared, write_error));
		if(write_error)
			fail(write_error);
		else
			finish_protocol_failure();
		return;
	}

	m_pending_protocol_close = std::move(*prepared);
	start_close_deadline();
	if(m_protocol_failure_active)
		schedule_send();
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::fail(error_code error) noexcept
{
	if(not m_error)
		m_error = error;
	if(m_close_timer)
	{
		try
		{
			ignore_unused(m_close_timer->cancel());
		}
		catch(...)
		{
		}
		m_close_timer.reset();
	}
	m_state = connection_state::failed;
	complete_read_waiter(m_error);
	stop_control_observer(m_error);
	if(not m_wire_write_active and m_current_data)
	{
		auto operation = std::exchange(m_current_data, {});
		complete_send_operation(operation, m_error);
	}
	fail_queued_writes(m_error);
	m_pending_auto_pong.reset();
	m_pending_local_close.reset();
	m_pending_close_response.reset();
	m_pending_protocol_close.reset();
	m_protocol_failure_active = false;
	complete_close_waiters(m_error);
	if(m_connection and not m_transport_closed)
	{
		ignore_unused(m_connection->cancel());
		ignore_unused(m_connection->close());
		m_transport_closed = true;
	}
}

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_LIFECYCLE_IPP
