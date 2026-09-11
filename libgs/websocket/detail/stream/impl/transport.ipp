// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_TRANSPORT_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_TRANSPORT_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
#error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif

namespace libgs::websocket
{

template <core_concepts::exec Exec>
size_t basic_stream<Exec>::impl::read_transport
(const mutable_buffer &buffer, error_code &error) noexcept
{
	return m_connection->read(buffer, error);
}

template <core_concepts::exec Exec>
awaitable<std::tuple<error_code,size_t>> basic_stream<Exec>::impl::async_read_transport
(std::shared_ptr<std::vector<std::byte>> storage)
{
	auto self = this->shared_from_this();
	co_return co_await http::detail::initiate_connection_io (
		[self, storage](auto next_handler) mutable
		{
			self->m_connection->read (
				mutable_buffer(storage->data(), storage->size()),
				asio::any_completion_handler<void(error_code,size_t)>
					(std::move(next_handler))
			);
		},
		asio::as_tuple(asio::use_awaitable)
	);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::start_transport_write
(prepared_frame frame, detail::wire_frame_kind kind,
	std::shared_ptr<detail::send_operation> operation) noexcept
{
	auto self = this->shared_from_this();
	try {
		m_connection->write(std::span<const const_buffer>(frame.buffers),
			asio::any_completion_handler<void(error_code, size_t)>(
			[self, frame, kind, operation](error_code error, size_t wire_size) mutable
			{
				self->m_send_engine.complete_wire_frame(std::move(frame), kind,
					std::move(operation), error, wire_size
				);
			}
		));
	}
	catch(...)
	{
		m_send_engine.complete_wire_frame(std::move(frame), kind,
			std::move(operation), exception_error(std::current_exception()), 0
		);
	}
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::close_transport(error_code &error) noexcept
{
	if( not m_connection or m_transport_closed )
	{
		error.clear();
		return ;
	}
	auto result = m_connection->close();
	m_transport_closed = true;

	error = result ?
		error_code{} : result.error();
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_TRANSPORT_IPP
