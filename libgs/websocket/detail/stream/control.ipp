// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_CONTROL_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_CONTROL_IPP

// Peer control frames and control observers.

template <core_concepts::exec Exec>
sys_expected<> stream_impl<Exec>::remember_peer_close(
	const std::vector<std::byte> &payload) noexcept
{
	auto decoded = decode_close_payload(const_buffer(
		payload.data(), payload.size()));
	if(not decoded)
		return sys_unexpected(decoded.error());
	try
	{
		m_peer_close = close_info_t{
			.code = decoded->code,
			.reason = std::string(decoded->reason),
			.clean = false,
		};
		m_state = connection_state::closing;
		m_control_event.reset();
		complete_control_waiter(make_error_code(errc::closing));
		return make_sys_expected();
	}
	catch(const std::bad_alloc &)
	{
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...)
	{
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
}

template <core_concepts::exec Exec>
error_code stream_impl<Exec>::control_state_error() const noexcept
{
	if(m_state == connection_state::failed)
		return m_error ? m_error : make_error_code(std::errc::io_error);
	if(m_state == connection_state::closed)
		return asio::error::eof;
	if(m_state == connection_state::closing)
		return make_error_code(errc::closing);
	if(m_state == connection_state::idle)
		return make_error_code(errc::not_open);
	return {};
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::complete_control_waiter(error_code error,
	control_event_t event, bool clear_slot) noexcept
{
	if(not m_control_waiter)
		return;
	auto waiter = std::exchange(m_control_waiter, {});
	if(clear_slot)
	{
		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
			slot.clear();
	}
	try
	{
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, std::move(event));
	}
	catch(...)
	{
		// Completion dispatch failure is local to the observer and must not
		// corrupt the receive parser or transport state.
	}
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::cancel_control_waiter(uint64_t id) noexcept
{
	if(m_control_waiter and m_control_waiter->id == id)
		complete_control_waiter(asio::error::operation_aborted, {}, false);
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::stop_control_observer(error_code error) noexcept
{
	m_control_event.reset();
	complete_control_waiter(error);
}

template <core_concepts::exec Exec>
void stream_impl<Exec>::remember_control(
	opcode op, std::vector<std::byte> payload) noexcept
{
	try
	{
		const auto type = op == opcode::ping ? control_type::ping : control_type::pong;
		control_event_t event{.type = type, .payload = std::move(payload)};
		if(m_control_waiter)
		{
			complete_control_waiter({}, std::move(event));
			return;
		}
		// A manual-Pong Ping must not be displaced by an unobserved Pong.
		if(not m_config.automatic_pong and op == opcode::pong and
			m_control_event and m_control_event->type == control_type::ping)
			return;
		m_control_event = std::move(event);
	}
	catch(...)
	{
		// Control observation is auxiliary to read(); failure to retain an
		// event must not corrupt the receive parser or the connection.
	}
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::wait_control(error_code &error) noexcept
	-> control_event_t
{
	if(m_control_waiter)
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}
	if(auto state_error = control_state_error())
	{
		error = state_error;
		return {};
	}
	if(not m_control_event)
	{
		// wait_ctrl never starts a transport read, so a synchronous caller
		// can only consume an event already parsed by read()/close().
		error = make_error_code(std::errc::operation_would_block);
		return {};
	}
	error.clear();
	return std::exchange(m_control_event, std::nullopt).value();
}

template <core_concepts::exec Exec>
template <typename Handler>
void stream_impl<Exec>::async_wait_control(Handler &&handler)
{
	auto completion =
		asio::any_completion_handler<void(error_code, control_event_t)>(
			std::forward<Handler>(handler));
	auto self = this->shared_from_this();
	if(self->m_control_waiter)
	{
		auto error = make_error_code(std::errc::operation_in_progress);
		asio::post(self->m_exec,
			[handler = std::move(completion), error]() mutable
			{
				std::move(handler)(error, control_event_t{});
			});
		return;
	}
	if(auto error = self->control_state_error())
	{
		asio::post(self->m_exec,
			[handler = std::move(completion), error]() mutable
			{
				std::move(handler)(error, control_event_t{});
			});
		return;
	}
	if(self->m_control_event)
	{
		auto event = std::exchange(self->m_control_event, std::nullopt).value();
		asio::post(self->m_exec,
			[handler = std::move(completion), event = std::move(event)]() mutable
			{
				std::move(handler)(error_code{}, std::move(event));
			});
		return;
	}

	try
	{
		auto associated_allocator =
			asio::get_associated_allocator(completion);
		using waiter_allocator_t = std::allocator_traits<
			decltype(associated_allocator)>::template rebind_alloc<control_wait_operation>;
		auto waiter = std::allocate_shared<control_wait_operation>(
			waiter_allocator_t(associated_allocator));
		waiter->id = ++self->m_next_control_waiter_id;
		waiter->completion = std::move(completion);
		self->m_control_waiter = waiter;
		auto slot = asio::get_associated_cancellation_slot(waiter->completion);
		if(slot.is_connected())
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id](asio::cancellation_type type) noexcept
				{
				if( type == asio::cancellation_type::none )
				return;
				if( auto locked = weak.lock() )
				{
					try
					{
						asio::dispatch(locked->m_exec, [locked, id] {
							locked->cancel_control_waiter(id);
						});
					}
					catch(...)
					{
					}
				} });
		}
	}

	catch(const std::bad_alloc &)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if(self->m_control_waiter)
			self->complete_control_waiter(error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, control_event_t{});
				});
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if(self->m_control_waiter)
			self->complete_control_waiter(error);
		else
			asio::post(self->m_exec,
				[handler = std::move(completion), error]() mutable
				{
					std::move(handler)(error, control_event_t{});
				});
	}
}

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_CONTROL_IPP
