// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_READ_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_READ_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
#error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif

namespace libgs::websocket
{

template <core_concepts::exec Exec>
template <typename Buffer>
basic_message<Buffer> basic_stream<Exec>::impl::convert_message(message value, error_code &error) noexcept
{
	try {
		auto type = value.type;
		if constexpr( std::same_as<Buffer, std::vector<std::byte>> )
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
	catch(const std::bad_alloc&) {
		error = make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {
		error = make_error_code(std::errc::io_error);
	}
	return {};
}

template <core_concepts::exec Exec>
message basic_stream<Exec>::impl::read(error_code &error) noexcept
{
	return m_receive_engine.read(error);
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_read_message(Handler &&handler)
{
	m_receive_engine.async_read_message(std::forward<Handler>(handler));
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_READ_IPP
