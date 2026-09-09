// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_DETAIL_WAKE_UP_H
#define LIBGS_CORO_DETAIL_WAKE_UP_H

#include <libgs/core/execution.h>

namespace libgs::coro::detail
{

class LIBGS_CORO_VAPI lock_wake_up final :
	public std::enable_shared_from_this<lock_wake_up>
{
	LIBGS_DISABLE_COPY_MOVE(lock_wake_up)

public:
	using ptr_t = std::shared_ptr<lock_wake_up>;
	using handler_t = async_work<bool>::handler_t;

	lock_wake_up(const asio::any_io_executor &exec, handler_t handler) :
		m_handler(std::move(handler)), m_exec(exec) {}

public:
	bool operator()(bool success)
	{
		if( m_finished.test_and_set() )
			return false;
		m_timer.cancel();

		dispatch(m_exec, [
			success, handler = std::make_shared<handler_t>(std::move(m_handler))
		]() mutable {
			std::move(*handler)(success);
		});
		return true;
	}

	void start_timer(const auto &timeout)
	{
		if constexpr( requires { timeout.time_since_epoch(); } )
		{
			using clock_t = typename std::remove_cvref_t<decltype(timeout)>::clock;
			auto now = clock_t::now();
			m_timer.expires_after(timeout <= now ? asio::steady_timer::duration::zero() :
				std::chrono::duration_cast<asio::steady_timer::duration>(timeout - now));
		}
		else
		{
			m_timer.expires_after(std::chrono::duration_cast<asio::steady_timer::duration>(timeout));
		}
		m_timer.async_wait([self = shared_from_this()](const error_code &error) mutable
		{
			if( not error )
				(*self)(false);
		});
	}

private:
	handler_t m_handler;
	asio::any_io_executor m_exec;
	asio::steady_timer m_timer{m_exec};
	std::atomic_flag m_finished {};
};

using lock_wake_up_ptr = lock_wake_up::ptr_t;

} //namespace libgs::coro::detail


#endif //LIBGS_CORO_DETAIL_WAKE_UP_H
