// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_DETAIL_MUTEX_H
#define LIBGS_CORO_DETAIL_MUTEX_H

#include <libgs/coro/detail/wake_up.h>
#include <deque>
#include <mutex>

namespace libgs::coro
{

class LIBGS_CORO_VAPI mutex::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using wake_up_t = detail::lock_wake_up;

	impl() = default;
	~impl()
	{
#if 0
		if( not m_native_handle )
			return ;
		runtime_error::loc_throw (
			"libgs::mutex: Destruct a mutex that has not yet been unlocked."
		);
#endif
	}

public:
	[[nodiscard]] bool try_lock()
	{
		bool flag = false;
		return m_native_handle.compare_exchange_strong(flag, true,
			std::memory_order_acquire, std::memory_order_relaxed
		);
	}

	void enqueue(const wake_up_t::ptr_t &waiter)
	{
		bool acquired = false;
		{
			// Close the failed-fast-path/enqueue gap against unlock().
			std::lock_guard guard(m_wait_mutex);
			if( try_lock() )
				acquired = true;
			else
				m_wait_queue.emplace_back(waiter);
		}
		if( acquired )
			(*waiter)(true);
	}

	void enqueue_timed(wake_up_t::ptr_t waiter, const auto &timeout)
	{
		bool acquired = false;
		{
			// Close the failed-fast-path/enqueue gap against unlock().
			std::lock_guard guard(m_wait_mutex);
			if( try_lock() )
				acquired = true;
			else
			{
				m_wait_queue.emplace_back(waiter);
				waiter->start_timer(timeout);
			}
		}
		if( acquired )
			(*waiter)(true);
	}

	void unlock()
	{
		for(;;)
		{
			wake_up_t::ptr_t waiter;
			{
				std::lock_guard guard(m_wait_mutex);
				if( m_wait_queue.empty() )
				{
					m_native_handle.store(false, std::memory_order_release);
					return ;
				}
				waiter = std::move(m_wait_queue.front());
				m_wait_queue.pop_front();
			}
			if( (*waiter)(true) )
				return ;
		}
	}

	[[nodiscard]] awaitable<bool> try_lock_x
	(concepts::sched auto &&exec, auto timeout)
	{
		if( try_lock() )
			co_return true;

		co_return co_await async_work<bool>::handle(exec,
		[this, timeout, wait_exec = get_executor_helper(exec)]
		(async_work<bool>::handler_t wake_up) mutable
		{
			auto wake_up_ptr = std::make_shared<wake_up_t>(wait_exec, std::move(wake_up));
			enqueue_timed(std::move(wake_up_ptr), timeout);
		});
	}

public:
	native_handle_t m_native_handle {false};
	std::mutex m_wait_mutex;
	std::deque<wake_up_t::ptr_t> m_wait_queue;
};

inline mutex::mutex() :
	m_impl(new impl())
{

}

inline mutex::~mutex()
{
	delete m_impl;
}

awaitable<void> mutex::lock(concepts::sched auto &&exec)
{
	if( try_lock() )
		co_return ;

	auto _exec = get_executor_helper (
		std::forward<decltype(exec)>(exec)
	);
	co_await async_work<bool>::handle(_exec,
	[this, _exec](async_work<bool>::handler_t wake_up) mutable
	{
		m_impl->enqueue (
			std::make_shared<impl::wake_up_t>(_exec, std::move(wake_up))
		);
	});
	co_return ;
}

inline awaitable<void> mutex::lock()
{
	co_return co_await lock (
		co_await asio::this_coro::executor
	);
}

inline bool mutex::try_lock()
{
	return m_impl->try_lock();
}

inline void mutex::unlock()
{
	m_impl->unlock();
}

template<typename Rep, typename Period>
awaitable<bool> mutex::try_lock_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_impl->try_lock_x(std::forward<decltype(exec)>(exec),
		std::chrono::duration_cast<asio::steady_timer::duration>(timeout)
	);
}

template<typename Clock, typename Duration>
awaitable<bool> mutex::try_lock_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_impl->try_lock_x(std::forward<decltype(exec)>(exec), timeout);
}

template<typename Rep, typename Period>
awaitable<bool> mutex::try_lock_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_lock_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<typename Clock, typename Duration>
awaitable<bool> mutex::try_lock_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_lock_until (
		co_await asio::this_coro::executor, timeout
	);
}

inline bool mutex::is_locked() const noexcept
{
	return m_impl->m_native_handle.load(std::memory_order_acquire);
}

inline mutex::native_handle_t &mutex::native_handle() noexcept
{
	return m_impl->m_native_handle;
}

template <typename Mutex>
unique_lock<Mutex>::unique_lock(mutex_t &mutex) :
	m_mutex(&mutex)
{

}

template <typename Mutex>
unique_lock<Mutex>::~unique_lock() noexcept(noexcept(m_mutex->unlock()))
{
	if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock();
	}
}

template <typename Mutex>
unique_lock<Mutex>::unique_lock(unique_lock &&other) noexcept
{
	m_mutex = other.m_mutex;
	m_owns = other.m_owns;
	other.m_mutex = nullptr;
	other.m_owns = false;
}

template <typename Mutex>
unique_lock<Mutex> &unique_lock<Mutex>::operator=(unique_lock &&other) noexcept
{
	if( this == &other )
		return *this;

	else if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock();
	}
	m_mutex = other.m_mutex;
	m_owns = other.m_owns;

	other.m_mutex = nullptr;
	other.m_owns = false;
	return *this;
}

template <typename Mutex>
awaitable<void> unique_lock<Mutex>::lock(concepts::sched auto &&exec)
{
	if( m_mutex and not m_owns )
	{
		co_await m_mutex->lock(exec);
		m_owns = true;
	}
	co_return ;
}

template <typename Mutex>
awaitable<void> unique_lock<Mutex>::lock()
{
	co_return co_await lock (
		co_await asio::this_coro::executor
	);
}

template <typename Mutex>
bool unique_lock<Mutex>::try_lock()
{
	if( not m_mutex )
		return false;
	else if( m_owns )
		return true;
	return m_owns = m_mutex->try_lock();
}

template <typename Mutex>
void unique_lock<Mutex>::unlock()
{
	if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock();
	}
}

template <typename Mutex>
template<typename Rep, typename Period>
awaitable<bool> unique_lock<Mutex>::try_lock_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	if( not m_mutex )
		co_return false;
	else if( m_owns )
		co_return true;

	m_owns = co_await m_mutex->try_lock_for(exec, timeout);
	co_return m_owns;
}

template <typename Mutex>
template<typename Clock, typename Duration>
awaitable<bool> unique_lock<Mutex>::try_lock_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	if( not m_mutex )
		co_return false;
	else if( m_owns )
		co_return true;

	m_owns = co_await m_mutex->try_lock_until(exec, timeout);
	co_return m_owns;
}

template <typename Mutex>
template<typename Rep, typename Period>
awaitable<bool> unique_lock<Mutex>::try_lock_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_lock_for (
		co_await asio::this_coro::executor, timeout
	);
}

template <typename Mutex>
template<typename Clock, typename Duration>
awaitable<bool> unique_lock<Mutex>::try_lock_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_lock_until (
		co_await asio::this_coro::executor, timeout
	);
}

template <typename Mutex>
bool unique_lock<Mutex>::is_locked() const noexcept
{
	return m_owns;
}

template <typename Mutex>
unique_lock<Mutex>::mutex_t *unique_lock<Mutex>::mutex() noexcept
{
	return m_mutex;
}

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_MUTEX_H
