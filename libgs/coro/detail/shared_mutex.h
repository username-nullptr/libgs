// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_DETAIL_SHARED_MUTEX_H
#define LIBGS_CORO_DETAIL_SHARED_MUTEX_H

namespace libgs::coro
{

inline shared_mutex::~shared_mutex() = default;

awaitable<void> shared_mutex::lock(concepts::sched auto &&exec)
{
	return m_native_handle.lock(exec);
}

inline awaitable<void> shared_mutex::lock()
{
	return m_native_handle.lock();
}

inline bool shared_mutex::try_lock()
{
	return m_native_handle.try_lock();
}

inline void shared_mutex::unlock()
{
	return m_native_handle.unlock();
}

awaitable<void> shared_mutex::lock_shared(concepts::sched auto &&exec)
{
	auto _exec = get_executor_helper (
		std::forward<decltype(exec)>(exec)
	);
	co_await m_read_gate.lock(_exec);

	if( m_read_count.load() == 0 )
		co_await m_native_handle.lock(_exec);

	m_read_count.fetch_add(1);
	m_read_gate.unlock();
	co_return ;
}

inline awaitable<void> shared_mutex::lock_shared()
{
	co_return co_await lock_shared(co_await asio::this_coro::executor);
}

inline bool shared_mutex::try_lock_shared()
{
	if( not m_read_gate.try_lock() )
		return false;

	if( m_read_count.load() == 0 and not m_native_handle.try_lock() )
	{
		m_read_gate.unlock();
		return false;
	}
	m_read_count.fetch_add(1);
	m_read_gate.unlock();
	return true;
}

inline void shared_mutex::unlock_shared()
{
	auto counter = m_read_count.load();
	while( counter > 0 )
	{
		if( m_read_count.compare_exchange_weak(counter, counter - 1) )
		{
			if( counter == 1 )
				m_native_handle.unlock();
			return ;
		}
	}
}

template<typename Rep, typename Period>
awaitable<bool> shared_mutex::try_lock_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_native_handle.try_lock_for (
		std::forward<decltype(exec)>(exec), timeout
	);
}

template<typename Clock, typename Duration>
awaitable<bool> shared_mutex::try_lock_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_native_handle.try_lock_until (
		std::forward<decltype(exec)>(exec), timeout
	);
}

template<typename Rep, typename Period>
awaitable<bool> shared_mutex::try_lock_for(const duration<Rep,Period> &timeout)
{
	return m_native_handle.try_lock_for(timeout);
}

template<typename Clock, typename Duration>
awaitable<bool> shared_mutex::try_lock_until(const time_point<Clock,Duration> &timeout)
{
	return m_native_handle.try_lock_until(timeout);
}

template<typename Rep, typename Period>
awaitable<bool> shared_mutex::try_lock_shared_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	auto atime = std::chrono::steady_clock::now() +
		std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);

	co_return co_await try_lock_shared_until(
		std::forward<decltype(exec)>(exec), atime
	);
}

template<typename Clock, typename Duration>
awaitable<bool> shared_mutex::try_lock_shared_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	auto _exec = get_executor_helper(std::forward<decltype(exec)>(exec));
	if( not co_await m_read_gate.try_lock_until(_exec, timeout) )
		co_return false;

	if( m_read_count.load() == 0 and
		not co_await m_native_handle.try_lock_until(_exec, timeout) )
	{
		m_read_gate.unlock();
		co_return false;
	}
	m_read_count.fetch_add(1);
	m_read_gate.unlock();
	co_return true;
}

template<typename Rep, typename Period>
awaitable<bool> shared_mutex::try_lock_shared_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_lock_shared_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<typename Clock, typename Duration>
awaitable<bool> shared_mutex::try_lock_shared_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_lock_shared_until (
		co_await asio::this_coro::executor, timeout
	);
}

inline bool shared_mutex::is_locked() const noexcept
{
	return m_native_handle.is_locked();
}

inline shared_mutex::native_handle_t &shared_mutex::native_handle() noexcept
{
	return m_native_handle;
}

inline shared_lock::shared_lock(mutex_t &mutex) :
	m_mutex(&mutex)
{

}

inline shared_lock::~shared_lock() noexcept(noexcept(m_mutex->unlock_shared()))
{
	if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock_shared();
	}
}

inline shared_lock::shared_lock(shared_lock &&other) noexcept :
	m_mutex(other.m_mutex), m_owns(other.m_owns)
{
	other.m_mutex = nullptr;
	other.m_owns = false;
}

inline shared_lock &shared_lock::operator=(shared_lock &&other) noexcept
{
	if( this == &other )
		return *this;

	else if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock_shared();
	}
	m_mutex = other.m_mutex;
	m_owns = other.m_owns;

	other.m_mutex = nullptr;
	other.m_owns = false;
	return *this;
}

awaitable<void> shared_lock::lock_shared(concepts::sched auto &&exec)
{
	if( m_mutex and not m_owns )
	{
		co_await m_mutex->lock_shared(exec);
		m_owns = true;
	}
	co_return ;
}

inline awaitable<void> shared_lock::lock_shared()
{
	co_return co_await lock_shared (
		co_await asio::this_coro::executor
	);
}

inline bool shared_lock::try_lock_shared()
{
	if( not m_mutex )
		return false;
	else if( m_owns )
		return true;
	return m_owns = m_mutex->try_lock_shared();
}

inline void shared_lock::unlock_shared()
{
	if( m_mutex and m_owns )
	{
		m_owns = false;
		m_mutex->unlock_shared();
	}
}

template<typename Rep, typename Period>
awaitable<bool> shared_lock::try_lock_shared_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	if( not m_mutex )
		co_return false;
	else if( m_owns )
		co_return true;

	m_owns = co_await m_mutex->try_lock_shared_for(exec, timeout);
	co_return m_owns;
}

template<typename Clock, typename Duration>
awaitable<bool> shared_lock::try_lock_shared_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	if( not m_mutex )
		co_return false;
	else if( m_owns )
		co_return true;

	m_owns = co_await m_mutex->try_lock_shared_until(exec, timeout);
	co_return m_owns;
}

template<typename Rep, typename Period>
awaitable<bool> shared_lock::try_lock_shared_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_lock_shared_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<typename Clock, typename Duration>
awaitable<bool> shared_lock::try_lock_shared_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_lock_shared_until (
		co_await asio::this_coro::executor, timeout
	);
}

inline bool shared_lock::is_locked() const noexcept
{
	return m_owns;
}

inline shared_lock::mutex_t *shared_lock::mutex() noexcept
{
	return m_mutex;
}

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_SHARED_MUTEX_H
