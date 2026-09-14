// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_CONTROL_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_CONTROL_IPP

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_H
# error "Include <libgs/websocket/detail/stream/receive_engine.h> instead."
#endif

namespace libgs::websocket
{

template <typename Owner>
error_code detail::receive_engine<Owner>::control_state_error() const noexcept
{
	return m_owner.control_state_error();
}

template <typename Owner>
void detail::receive_engine<Owner>::complete_control_waiter
(error_code error, control_event event, bool clear_slot) noexcept
{
	if( not m_control_waiter )
		return ;

	auto waiter = std::exchange(m_control_waiter, {});
	if( clear_slot )
	{
		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion);
			slot.is_connected() )
			slot.clear();
	}
	try {
		auto completion = std::move(waiter->completion);
		std::move(completion)(error, std::move(event));
	}
	catch(...) {}
}

template <typename Owner>
void detail::receive_engine<Owner>::cancel_control_waiter(uint64_t id) noexcept
{
	if( m_control_waiter and m_control_waiter->id == id )
		complete_control_waiter(asio::error::operation_aborted, {}, false);
}

template <typename Owner>
void detail::receive_engine<Owner>::stop_control_observer(error_code error) noexcept
{
	m_control_event.reset();
	complete_control_waiter(error);
}

template <typename Owner>
void detail::receive_engine<Owner>::remember_control(opcode op, std::vector<std::byte> payload) noexcept
{
	try {
		const auto type = op == opcode::ping ?
			control_type::ping : control_type::pong;

		control_event event {
			.type = type, .payload = std::move(payload)
		};
		if( m_control_waiter )
		{
			complete_control_waiter({}, std::move(event));
			return ;
		}
		if( not m_owner.automatic_pong_enabled() and op == opcode::pong and
			m_control_event and m_control_event->type == control_type::ping )
			return ;
		m_control_event = std::move(event);
	}
	catch(...) {}
}

template <typename Owner>
control_event detail::receive_engine<Owner>::wait_control(error_code &error) noexcept
{
	if( m_control_waiter )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return {};
	}
	if( auto state_error = control_state_error() )
	{
		error = state_error;
		return {};
	}
	if( not m_control_event )
	{
		error = make_error_code(std::errc::operation_would_block);
		return {};
	}
	error.clear();
	return std::exchange(m_control_event, nullopt).value();
}

template <typename Owner>
template <typename Handler>
void detail::receive_engine<Owner>::async_wait_control(Handler &&handler)
{
	auto completion = asio::any_completion_handler
		<void(error_code, control_event)>(std::forward<Handler>(handler));

	auto self = m_owner.shared_from_this();
	if( self->receive_side().m_control_waiter )
	{
		auto error = make_error_code(std::errc::operation_in_progress);
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, control_event{});
		});
		return ;
	}
	if( auto error = self->receive_side().control_state_error() )
	{
		asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
			std::move(handler)(error, control_event{});
		});
		return ;
	}
	if( self->receive_side().m_control_event )
	{
		auto event = std::exchange(self->receive_side().m_control_event, nullopt).value();
		asio::post(self->executor(),
		[handler = std::move(completion), event = std::move(event)]() mutable {
			std::move(handler)(error_code{}, std::move(event));
		});
		return ;
	}
	try {
		auto associated_allocator = asio::get_associated_allocator(completion);

		using waiter_allocator_t = std::allocator_traits
			<decltype(associated_allocator)>::template rebind_alloc<control_wait_operation>;

		auto waiter = std::allocate_shared
			<control_wait_operation>(waiter_allocator_t(associated_allocator));

		waiter->id = ++self->receive_side().m_next_control_waiter_id;
		waiter->completion = std::move(completion);

		self->receive_side().m_control_waiter = waiter;

		if( auto slot = asio::get_associated_cancellation_slot(waiter->completion); slot.is_connected() )
		{
			slot.assign([weak = self->weak_from_this(), id = waiter->id]
			(asio::cancellation_type type) noexcept
			{
				if( type == asio::cancellation_type::none )
					return ;

				if( auto locked = weak.lock() )
				{
					try {
						asio::dispatch(locked->executor(), [locked, id]{
							locked->receive_side().cancel_control_waiter(id);
						});
					}
					catch(...) {}
				}
			});
		}
	}
	catch(const std::bad_alloc&)
	{
		auto error = make_error_code(std::errc::not_enough_memory);
		if( self->receive_side().m_control_waiter )
			self->receive_side().complete_control_waiter(error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, control_event{});
			});
		}
	}
	catch(...)
	{
		auto error = make_error_code(std::errc::io_error);
		if( self->receive_side().m_control_waiter )
			self->receive_side().complete_control_waiter(error);
		else
		{
			asio::post(self->executor(), [handler = std::move(completion), error]() mutable {
				std::move(handler)(error, control_event{});
			});
		}
	}
}

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_CONTROL_IPP
