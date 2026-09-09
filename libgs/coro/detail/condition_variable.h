// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_DETAIL_CONDITION_VARIABLE_H
#define LIBGS_CORO_DETAIL_CONDITION_VARIABLE_H

#include <deque>
#include <mutex>

namespace libgs::coro
{

class LIBGS_CORO_VAPI condition_variable::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;

	template <typename Mutex>
	[[nodiscard]] awaitable<bool> wait
	(concepts::sched auto &&exec, unique_lock<Mutex> &lock)
	{
		auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
		co_return co_await async_work<bool>::handle(_exec,
		[this, &lock, _exec](async_work<bool>::handler_t wake_up) mutable
		{
			auto waiter = std::make_shared<detail::lock_wake_up>(
				_exec, std::move(wake_up)
			);
			std::lock_guard guard(m_mutex);
			m_wait_queue.emplace_back(std::move(waiter));
			lock.unlock();
		});
	}

	template <typename Mutex, typename Timeout>
	[[nodiscard]] awaitable<bool> wait
	(concepts::sched auto &&exec, unique_lock<Mutex> &lock, Timeout timeout)
	{
		auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
		co_return co_await async_work<bool>::handle(_exec,
		[this, &lock, wait_timeout = std::move(timeout), _exec]
		(async_work<bool>::handler_t wake_up) mutable
		{
			auto waiter = std::make_shared<detail::lock_wake_up>(
				_exec, std::move(wake_up)
			);
			std::lock_guard guard(m_mutex);
			m_wait_queue.emplace_back(waiter);
			waiter->start_timer(wait_timeout);
			lock.unlock();
		});
	}

	void notify_one() noexcept
	{
		for(;;)
		{
			detail::lock_wake_up_ptr waiter;
			{
				std::lock_guard guard(m_mutex);
				if( m_wait_queue.empty() )
					return ;
				waiter = std::move(m_wait_queue.front());
				m_wait_queue.pop_front();
			}
			if( (*waiter)(true) )
				return ;
		}
	}

	void notify_all() noexcept
	{
		std::deque<detail::lock_wake_up_ptr> waiters;
		{
			std::lock_guard guard(m_mutex);
			waiters.swap(m_wait_queue);
		}
		for(auto &waiter : waiters)
			(*waiter)(true);
	}

public:
	std::deque<detail::lock_wake_up_ptr> m_wait_queue;
	std::mutex m_mutex;
};

inline condition_variable::condition_variable() :
	m_impl(new impl())
{

}

inline condition_variable::~condition_variable()
{
	delete m_impl;
}

template <typename Mutex>
awaitable<void> condition_variable::wait(unique_lock<Mutex> &lock) noexcept
{
	co_return co_await wait(co_await asio::this_coro::executor, lock);
}

template <typename Mutex>
awaitable<void> condition_variable::wait(unique_lock<Mutex> &lock, auto pred)
{
	co_return co_await wait(co_await asio::this_coro::executor, lock, std::move(pred));
}

template <typename Mutex>
awaitable<void> condition_variable::wait
(concepts::sched auto &&exec, unique_lock<Mutex> &lock) noexcept
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	co_await m_impl->wait(_exec, lock);
	co_await lock.lock(_exec);
	co_return ;
}

template <typename Mutex>
awaitable<void> condition_variable::wait
(concepts::sched auto &&exec, unique_lock<Mutex> &lock, auto pred)
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	while( not pred() )
		co_await wait(_exec, lock);
	co_return ;
}

inline void condition_variable::notify_one() noexcept
{
	m_impl->notify_one();
}

inline void condition_variable::notify_all() noexcept
{
	m_impl->notify_all();
}

template <typename Mutex, typename Rep, typename Period>
awaitable<bool> condition_variable::wait_for
(unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime)
{
	co_return co_await wait_for (
		co_await asio::this_coro::executor, lock, rtime
	);
}

template <typename Mutex, typename Rep, typename Period>
awaitable<bool> condition_variable::wait_for
(unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime, auto pred)
{
	co_return co_await wait_for (
		co_await asio::this_coro::executor, lock, rtime, std::move(pred)
	);
}

template <typename Mutex, typename Clock, typename Duration>
awaitable<bool> condition_variable::wait_until
(unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime)
{
	co_return co_await wait_until (
		co_await asio::this_coro::executor, lock, atime
	);
}

template <typename Mutex, typename Clock, typename Duration>
awaitable<bool> condition_variable::wait_until
(unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime, auto pred)
{
	co_return co_await wait_until (
		co_await asio::this_coro::executor, lock, atime, std::move(pred)
	);
}

template <typename Mutex, typename Rep, typename Period>
awaitable<bool> condition_variable::wait_for
(concepts::sched auto &&exec, unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime)
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	auto notified = co_await m_impl->wait(_exec, lock,
		std::chrono::duration_cast<asio::steady_timer::duration>(rtime)
	);
	co_await lock.lock(_exec);
	co_return notified;
}

template <typename Mutex, typename Rep, typename Period>
awaitable<bool> condition_variable::wait_for
(concepts::sched auto &&exec, unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime, auto pred)
{
	auto atime = std::chrono::steady_clock::now() +
		std::chrono::duration_cast<std::chrono::steady_clock::duration>(rtime);

	co_return co_await wait_until(
		std::forward<decltype(exec)>(exec), lock, atime, std::move(pred)
	);
}

template <typename Mutex, typename Clock, typename Duration>
awaitable<bool> condition_variable::wait_until
(concepts::sched auto &&exec, unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime)
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	auto notified = co_await m_impl->wait(_exec, lock, atime);
	co_await lock.lock(_exec);
	co_return notified;
}

template <typename Mutex, typename Clock, typename Duration>
awaitable<bool> condition_variable::wait_until
(concepts::sched auto &&exec, unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime, auto pred)
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	while( not pred() )
	{
		if( not co_await wait_until(_exec, lock, atime) )
			co_return pred();
	}
	co_return true;
}

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_CONDITION_VARIABLE_H
