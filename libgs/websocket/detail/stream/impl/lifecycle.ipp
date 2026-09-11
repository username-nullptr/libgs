// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_LIFECYCLE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_LIFECYCLE_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
#error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif

namespace libgs::websocket
{

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::cancel(error_code &error) noexcept
{
	if( not m_connection or m_transport_closed )
	{
		error.clear();
		return ;
	}
	auto result = m_connection->cancel();
	error = result ? error_code{} : result.error();
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::shutdown(error_code &error) noexcept
{
	if( m_close_timer )
	{
		try {
			ignore_unused(m_close_timer->cancel());
		}
		catch(...) {}
		m_close_timer.reset();
	}
	if( m_state == connection_state::closed )
	{
		m_receive_engine.complete_read_waiter(asio::error::operation_aborted);
		m_receive_engine.stop_control_observer(asio::error::operation_aborted);

		complete_close_waiters(asio::error::operation_aborted);
		error.clear();
		return ;
	}
	if( not m_connection or m_transport_closed )
	{
		m_close_result = retained_close_info(false);
		m_state = connection_state::closed;

		m_receive_engine.complete_read_waiter(asio::error::operation_aborted);
		m_receive_engine.stop_control_observer(asio::error::operation_aborted);
		complete_close_waiters(asio::error::operation_aborted);

		error.clear();
		return ;
	}
	m_receive_engine.complete_read_waiter(asio::error::operation_aborted);
	m_receive_engine.stop_control_observer(asio::error::operation_aborted);

	m_send_engine.fail_queued_writes(asio::error::operation_aborted);
	m_send_engine.clear_protocol_frames();

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
optional<close_code> basic_stream<Exec>::impl::protocol_failure_code(error_code error) noexcept
{
	if( error.category() == protocol_error_category() )
		return close_code_for(static_cast<protocol_errc>(error.value()));

	if( error == errc::message_too_big )
		return close_code::message_too_big;
	return nullopt;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::finish_protocol_failure(bool cancel_transport) noexcept
{
	if( not m_protocol_failure_active )
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

	m_send_engine.clear_protocol_close();
	m_protocol_failure_active = false;

	error_code close_error;
	close_transport(close_error);

	m_close_result = retained_close_info(false);
	m_state = connection_state::failed;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::begin_protocol_failure(error_code error, bool synchronous) noexcept
{
	auto code = protocol_failure_code(error);
	if( not code or m_state != connection_state::open or local_close_sent() or m_transport_closed )
	{
		fail(error);
		return ;
	}
	if( not m_error )
		m_error = error;

	m_state = connection_state::failed;
	m_protocol_failure_active = true;
	m_close_result = retained_close_info(false);

	m_receive_engine.complete_read_waiter(m_error);
	m_receive_engine.stop_control_observer(m_error);
	complete_close_waiters(m_error);

	m_send_engine.fail_queued_writes(m_error);
	m_send_engine.clear_automatic_pong();
	m_send_engine.clear_local_close();
	m_send_engine.clear_close_response();
	m_send_engine.fail_current_if_idle(m_error);

	auto prepared = m_send_engine.prepare_close(close_frame(*code));
	if( not prepared )
	{
		fail(prepared.error());
		return ;
	}
	if( m_config.close_timeout <= std::chrono::milliseconds::zero() )
	{
		finish_protocol_failure(true);
		return ;
	}
	if( synchronous and m_send_engine.ready_for_sync_protocol_write() )
	{
		error_code write_error;
		ignore_unused(write_prepared(*prepared, write_error));

		if( write_error )
			fail(write_error);
		else
			finish_protocol_failure();
		return ;
	}
	m_send_engine.queue_protocol_close(std::move(*prepared));
	start_close_deadline();

	if( m_protocol_failure_active )
		m_send_engine.schedule();
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::fail(error_code error) noexcept
{
	if( not m_error )
		m_error = error;

	if( m_close_timer )
	{
		try {
			ignore_unused(m_close_timer->cancel());
		}
		catch(...) {}
		m_close_timer.reset();
	}
	m_state = connection_state::failed;
	m_receive_engine.complete_read_waiter(m_error);
	m_receive_engine.stop_control_observer(m_error);

	m_send_engine.fail_current_if_idle(m_error);
	m_send_engine.fail_queued_writes(m_error);
	m_send_engine.clear_protocol_frames();
	m_protocol_failure_active = false;

	complete_close_waiters(m_error);
	if( m_connection and not m_transport_closed )
	{
		ignore_unused(m_connection->cancel());
		ignore_unused(m_connection->close());
		m_transport_closed = true;
	}
}

template <core_concepts::exec Exec>
error_code basic_stream<Exec>::impl::finish_receive_eof() noexcept
{
	error_code close_error;
	close_transport(close_error);

	if( close_error )
	{
		fail(close_error);
		return close_error;
	}
	m_close_result = retained_close_info(false);
	m_state = connection_state::closed;

	m_receive_engine.stop_control_observer(asio::error::eof);
	complete_close_waiters(asio::error::eof);
	return asio::error::eof;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::handle_send_failure(error_code error) noexcept
{
	fail(error);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::handle_receive_failure(error_code error) noexcept
{
	fail(error);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::handle_receive_protocol_failure(error_code error, bool synchronous) noexcept
{
	begin_protocol_failure(error, synchronous);
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_LIFECYCLE_IPP
