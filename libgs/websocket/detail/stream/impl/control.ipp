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
sys_expected<> basic_stream<Exec>::impl::handle_sync_ping(ctrl_payload_t &payload) noexcept
{
	if( not automatic_control_enabled() )
		return make_sys_expected();

	if( m_send_engine.busy() )
		return queue_auto_pong(payload.storage());

	auto frame = m_send_engine.prepare_control(opcode::pong,
		payload.as_const_buffer()
	);
	if( not frame )
		return sys_unexpected(frame.error());

	error_code error;
	ignore_unused(write_prepared(*frame, error));
	return error ? sys_expected<>(sys_unexpected(error)) : make_sys_expected();
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::handle_sync_control
(opcode op, std::vector<std::byte> &payload) noexcept
{
	if( op == opcode::pong )
		acknowledge_automatic_pong(payload);

	auto &callback = op == opcode::ping ? m_on_ping : m_on_pong;

	if( auto &async_callback = op == opcode::ping ?
		m_on_async_ping : m_on_async_pong; async_callback )
	{
		return sys_unexpected (
			make_error_code(std::errc::operation_not_supported)
		);
	}
	try {
		ctrl_payload_t callback_payload(std::move(payload));
		if( callback )
			callback(callback_payload);

		return op == opcode::ping ?
			handle_sync_ping(callback_payload) : make_sys_expected();
	}
	catch(...) {
		return sys_unexpected(exception_error(std::current_exception()));
	}
}

template <core_concepts::exec Exec>
awaitable<error_code> basic_stream<Exec>::impl::handle_async_control
(opcode op, std::vector<std::byte> &payload)
{
	if( op == opcode::pong )
		acknowledge_automatic_pong(payload);

	auto &callback = op == opcode::ping ? m_on_ping : m_on_pong;
	auto &async_callback = op == opcode::ping ? m_on_async_ping : m_on_async_pong;
	try {
		ctrl_payload_t callback_payload(std::move(payload));
		if( async_callback )
			co_await async_callback(callback_payload);

		else if( callback )
			callback(callback_payload);

		if( op == opcode::ping and automatic_control_enabled() )
		{
			auto queued = queue_auto_pong(callback_payload.storage());
			co_return queued ? error_code{} : queued.error();
		}
	}
	catch(...) {
		co_return exception_error(std::current_exception());
	}
	co_return error_code{};
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_ping(sync_control_callback_t callback)
{
	m_on_ping = std::move(callback);
	m_on_async_ping = {};
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_ping(async_control_callback_t callback)
{
	m_on_ping = {};
	m_on_async_ping = std::move(callback);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_pong(sync_control_callback_t callback)
{
	m_on_pong = std::move(callback);
	m_on_async_pong = {};
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::on_pong(async_control_callback_t callback)
{
	m_on_pong = {};
	m_on_async_pong = std::move(callback);
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::start_automatic_ping() noexcept
{
	if( not automatic_control_enabled() )
		return make_sys_expected();

	error_code error;
	try {
		m_ping_timer = std::make_shared<asio::steady_timer>(m_exec);
		if( auto scheduled = schedule_automatic_ping(); not scheduled )
		{
			m_ping_timer.reset();
			return scheduled;
		}
		return make_sys_expected();
	}
	catch(...) {
		error = exception_error(std::current_exception());
	}
	m_ping_timer.reset();
	return sys_unexpected(error);
}

template <core_concepts::exec Exec>
sys_expected<> basic_stream<Exec>::impl::schedule_automatic_ping() noexcept
{
	if( not m_ping_timer or m_state != connection_state::open )
		return make_sys_expected();

	error_code error;
	try {
		auto timer = m_ping_timer;
		timer->expires_after(m_config.ping_interval);

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
				if( self->m_awaited_pong )
				{
					if( self->m_consecutive_pong_timeouts >=
						self->m_config.pong_timeout_retries )
					{
						self->fail(asio::error::timed_out);
						return ;
					}
					++self->m_consecutive_pong_timeouts;
				}
				auto payload = std::make_shared<std::array<std::byte,8>>();
				auto id = ++self->m_next_ping_id;

				for(size_t index=0; index<payload->size(); ++index)
				{
					(*payload)[payload->size() - index - 1] =
						static_cast<std::byte>(id & 0xFF);
					id >>= 8;
				}
				self->m_awaited_pong = *payload;
				const auto body = const_buffer(payload->data(), payload->size());

				self->async_write_control(opcode::ping, body,
				[weak = std::move(weak), timer, payload](error_code write_error, size_t) mutable
				{
					if( auto locked = weak.lock(); locked and locked->m_ping_timer == timer and
						locked->m_state == connection_state::open )
					{
						if( write_error )
							return ;

						if( auto scheduled = locked->schedule_automatic_ping();
							not scheduled )
							locked->fail(scheduled.error());
					}
				},
				true);
			}
			catch(...) {
				self->fail(exception_error(std::current_exception()));
			}
		});
		return make_sys_expected();
	}
	catch(...) {
		error = exception_error(std::current_exception());
	}
	return sys_unexpected(error);
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::acknowledge_automatic_pong(const std::vector<std::byte> &payload) noexcept
{
	if( not m_awaited_pong or payload.size() != m_awaited_pong->size() or
		not std::equal(payload.begin(), payload.end(), m_awaited_pong->begin()) )
		return ;

	m_awaited_pong.reset();
	m_consecutive_pong_timeouts = 0;
}

template <core_concepts::exec Exec>
void basic_stream<Exec>::impl::stop_automatic_ping() noexcept
{
	auto timer = std::exchange(m_ping_timer, {});
	m_awaited_pong.reset();
	m_consecutive_pong_timeouts = 0;
	try {
		if( timer )
			ignore_unused(timer->cancel());
	}
	catch(...) {}
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_CONTROL_IPP
