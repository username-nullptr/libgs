
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_CORO_DETAIL_MUTEX_H
#define LIBGS_CORO_DETAIL_MUTEX_H

#include <libgs/core/lock_free_queue.h>
#include <libgs/coro/detail/wake_up.h>

namespace libgs::coro
{

class LIBGS_CORE_VAPI mutex::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using wake_up_t = detail::lock_wake_up;

	impl() = default;
	~impl() noexcept(false)
	{
		if( not m_native_handle )
			return ;
		throw runtime_error (
			"libgs::mutex: Destruct a mutex that has not yet been unlocked."
		);
	}

public:
	[[nodiscard]] bool try_lock()
	{
		bool flag = false;
		/*
			if( m_native_handle == flag )
	 		{
	 			m_native_handle = true;
	 			return true;
			}
	 		else
	 		{
	 			flag = m_native_handle;
				return false;
	 		}
		*/
		return m_native_handle.compare_exchange_weak(flag, true);
	}

	[[nodiscard]] awaitable<bool> try_lock_x
	(concepts::sched auto &&exec, const auto &timeout)
	{
		if( try_lock() )
			co_return true;

		co_return co_await async_work<bool>::handle(exec,
		[this, timeout, exec = get_executor_helper(exec)](auto &&wake_up) mutable
		{
			auto wake_up_ptr = std::make_shared<wake_up_t>(exec, std::move(wake_up));
			m_wait_queue.emplace(wake_up_ptr);
			wake_up_ptr->start_timer(timeout);
		});
	}

public:
	native_handle_t m_native_handle {false};
	lock_free_queue<detail::lock_wake_up_ptr> m_wait_queue;
};

inline mutex::mutex() :
	m_impl(new impl())
{

}

inline mutex::~mutex() noexcept(false)
{
	delete m_impl;
}

awaitable<void> mutex::lock(concepts::sched auto &&exec)
{
	if( try_lock() )
		co_return ;

	co_await async_work<bool>::handle(exec,
	[this, exec = get_executor_helper(exec)](async_work<bool>::handler_t wake_up) mutable
	{
		m_impl->m_wait_queue.emplace (
			std::make_shared<impl::wake_up_t>(exec, std::move(wake_up))
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
	for(;;)
	{
		auto wake_up = m_impl->m_wait_queue.dequeue();
		if( wake_up )
		{
			if( not std::move(**wake_up)(true) )
				continue;
		}
		else
			m_impl->m_native_handle = false;
		break;
	}
}

template<typename Rep, typename Period>
awaitable<bool> mutex::try_lock_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_impl->try_lock_x(exec,
		std::chrono::duration_cast<asio::steady_timer::duration>(timeout)
	);
}

template<typename Clock, typename Duration>
awaitable<bool> mutex::try_lock_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_impl->try_lock_x(exec,
		std::chrono::time_point_cast<asio::steady_timer::time_point>(timeout)
	);
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
	return m_impl->m_native_handle;
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
	if( m_mutex )
		m_mutex->unlock();
}

template <typename Mutex>
unique_lock<Mutex>::unique_lock(unique_lock &&other) noexcept
{
	m_mutex = other.m_mutex;
	other.m_mutex = nullptr;
}

template <typename Mutex>
unique_lock<Mutex> &unique_lock<Mutex>::operator=(unique_lock &&other) noexcept
{
	if( this == &other )
		return *this;
	else if( m_mutex )
		m_mutex->unlock();

	m_mutex = other.m_mutex;
	other.m_mutex = nullptr;
	return *this;
}

template <typename Mutex>
awaitable<void> unique_lock<Mutex>::lock(concepts::sched auto &&exec)
{
	if( m_mutex )
		co_await m_mutex->lock(exec);
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
	return m_mutex ? m_mutex->try_lock() : true;
}

template <typename Mutex>
void unique_lock<Mutex>::unlock()
{
	if( m_mutex )
		m_mutex->unlock();
}

template <typename Mutex>
template<typename Rep, typename Period>
awaitable<bool> unique_lock<Mutex>::try_lock_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	co_return m_mutex ? co_await m_mutex->try_lock_for(exec, timeout) : true;
}

template <typename Mutex>
template<typename Clock, typename Duration>
awaitable<bool> unique_lock<Mutex>::try_lock_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	co_return m_mutex ? co_await m_mutex->try_lock_until(exec, timeout) : true;
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
	return m_mutex ? m_mutex->is_locked() : false;
}

template <typename Mutex>
typename unique_lock<Mutex>::mutex_t *unique_lock<Mutex>::mutex() noexcept
{
	return m_mutex;
}

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_MUTEX_H