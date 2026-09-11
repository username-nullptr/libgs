// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CONTROL_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CONTROL_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
# error "Include <libgs/websocket/detail/stream/impl.h> instead."
#endif

namespace libgs::websocket
{

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::handle_sync_ping
(const std::vector<std::byte> &payload) noexcept
{
	if( not automatic_pong_enabled() )
		return make_sys_expected();

	if( m_send_engine.busy() )
		return queue_automatic_pong(payload);

	auto frame = m_send_engine.prepare_control(opcode::pong,
		const_buffer(payload.data(), payload.size())
	);
	if( not frame )
		return sys_unexpected(frame.error());

	error_code error;
	ignore_unused(write_prepared(*frame, error));
	return error ? sys_expected<>(sys_unexpected(error)) : make_sys_expected();
}

template <core_concepts::exec Exec>
auto basic_stream<Exec>::impl::wait_control(error_code &error) noexcept -> control_event_t
{
	return m_receive_engine.wait_control(error);
}

template <core_concepts::exec Exec>
template <typename Handler>
void basic_stream<Exec>::impl::async_wait_control(Handler &&handler)
{
	m_receive_engine.async_wait_control(std::forward<Handler>(handler));
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CONTROL_IPP
