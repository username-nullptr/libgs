// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_WRITE_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_WRITE_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
# error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif

namespace libgs::websocket
{

template <core_concepts::exec Exec>
size_t basic_stream<Exec>::impl::write
(message_type type, std::span<const const_buffer> buffers, error_code &error) noexcept
{
	error.clear();
	if( m_state == connection_state::idle )
	{
		error = make_error_code(errc::not_open);
		return 0;
	}
	if( m_state == connection_state::closing )
	{
		error = make_error_code(errc::closing);
		return 0;
	}
	if( m_state == connection_state::closed )
	{
		error = make_error_code(errc::closed);
		return 0;
	}
	if( m_state == connection_state::failed )
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return 0;
	}
	if( type != message_type::text and type != message_type::binary )
	{
		error = make_error_code(std::errc::invalid_argument);
		return 0;
	}
	if( m_send_engine.busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}
	auto prepared = m_send_engine.prepare_message(type, buffers);
	if( not prepared )
	{
		error = prepared.error();
		return 0;
	}
	size_t body_transferred = 0;
	for(const auto &frame : *prepared)
	{
		body_transferred += write_prepared(frame, error);
		if( error )
		{
			fail(error);
			return body_transferred;
		}
	}
	error.clear();
	return body_transferred;
}

template <core_concepts::exec Exec>
size_t basic_stream<Exec>::impl::write_prepared
(const prepared_frame &frame, error_code &error) noexcept
{
	if( m_send_engine.busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}
	const auto wire_size = m_connection->write (
		std::span(frame.buffers), error
	);
	const auto payload_size = wire_size > frame.header_size ?
		std::min(frame.payload_size, wire_size - frame.header_size) : 0;

	if( not error and wire_size != frame.header_size + frame.payload_size )
		error = make_error_code(std::errc::io_error);
	return payload_size;
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_write_message
(message_type type, std::span<const const_buffer> buffers,
	std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler)
{
	m_send_engine.async_write_message(type, buffers,
		std::move(payload_owner), std::forward<Handler>(handler)
	);
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_write_control
(opcode op, const const_buffer &payload, Handler &&handler)
{
	m_send_engine.async_write_control(op, payload, std::forward<Handler>(handler));
}

template <core_concepts::exec Exec>
size_t basic_stream<Exec>::impl::write_control
(opcode op, const const_buffer &payload, error_code &error) noexcept
{
	return m_send_engine.write_control(op, payload, error);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::wait_written(error_code &error) noexcept
{
	m_send_engine.wait_written(error);
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_wait_written(Handler &&handler)
{
	m_send_engine.async_wait_written(std::forward<Handler>(handler));
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::queue_automatic_pong
(const std::vector<std::byte> &payload) noexcept
{
	return m_send_engine.queue_automatic_pong(payload);
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_WRITE_IPP
