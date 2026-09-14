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
	if( not auto_pong_enabled() )
		return make_sys_expected();

	if( m_send_engine.busy() )
		return queue_auto_pong(payload);

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
sys_expected<> basic_stream<Exec>::impl::handle_sync_control
(opcode op, const std::vector<std::byte> &payload) noexcept
{
	auto accepted = accept_control(op, payload);
	if( not accepted )
		return sys_unexpected(accepted.error());

	if( not *accepted or op == opcode::pong )
		return make_sys_expected();

	return handle_sync_ping(payload);
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::handle_async_control
(opcode op, const std::vector<std::byte> &payload) noexcept
{
	auto accepted = accept_control(op, payload);
	if( not accepted )
		return sys_unexpected(accepted.error());

	if( not *accepted or op == opcode::pong or not auto_pong_enabled() )
		return make_sys_expected();

	return queue_auto_pong(payload);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_ping(control_callback_t callback)
{
	m_on_ping = std::move(callback);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_pong(control_callback_t callback)
{
	m_on_pong = std::move(callback);
}

template <core_concepts::exec Exec>
sys_expected<bool> basic_stream<Exec>::impl::accept_control
(opcode op, const std::vector<std::byte> &payload) noexcept
{
	try {
		auto &callback = op == opcode::ping ? m_on_ping : m_on_pong;
		if( not callback )
			return true;
		return callback(const_buffer(payload.data(), payload.size()));
	}
	catch(...) {}
	return sys_unexpected(exception_error(std::current_exception()));
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::start_automatic_ping() noexcept
{
	if( m_config.auto_ping_interval <= std::chrono::milliseconds::zero() )
		return make_sys_expected();
	try {
		m_ping_timer = std::make_shared<asio::steady_timer>(m_exec);
		if( auto scheduled = schedule_automatic_ping(); not scheduled )
		{
			m_ping_timer.reset();
			return scheduled;
		}
		return make_sys_expected();
	}
	catch(...) {}
	m_ping_timer.reset();
	return sys_unexpected(exception_error(std::current_exception()));
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::schedule_automatic_ping() noexcept
{
	if( not m_ping_timer or m_state != connection_state::open )
		return make_sys_expected();
	try {
		auto timer = m_ping_timer;
		timer->expires_after(m_config.auto_ping_interval);
		timer->async_wait (
		[weak = this->weak_from_this(), timer](error_code error) mutable
		{
			if( error )
				return ;
			auto self = weak.lock();

			if( not self or self->m_ping_timer != timer or
				self->m_state != connection_state::open )
				return ;
			try {
				self->async_write_control(opcode::ping, const_buffer{},
				[weak = std::move(weak), timer](error_code, size_t) mutable
				{
					if( auto locked = weak.lock(); locked and
						locked->m_ping_timer == timer and
						locked->m_state == connection_state::open )
					{
						if( auto scheduled = locked->schedule_automatic_ping();
							not scheduled )
							locked->fail(scheduled.error());
					}
				});
			}
			catch(...) {
				self->fail(exception_error(std::current_exception()));
			}
		});
		return make_sys_expected();
	}
	catch(...) {}
	return sys_unexpected(exception_error(std::current_exception()));
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::stop_automatic_ping() noexcept
{
	auto timer = std::exchange(m_ping_timer, {});
	if( not timer )
		return ;
	try {
		ignore_unused(timer->cancel());
	}
	catch(...) {}
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CONTROL_IPP
