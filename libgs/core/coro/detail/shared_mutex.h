
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

#ifndef LIBGS_CORE_CORO_DETAIL_SHARED_MUTEX_H
#define LIBGS_CORE_CORO_DETAIL_SHARED_MUTEX_H

namespace libgs
{

inline co_shared_mutex::~co_shared_mutex() noexcept(false)
{
	if( m_read_count == 0 )
		return ;
	throw runtime_error (
		"libgs::co_shared_mutex: Destruct a co_mutex that has not yet been unlock_shared."
	);
}

awaitable<void> co_shared_mutex::lock(concepts::schedulable auto &&exec)
{
	return m_native_handle.lock(exec);
}

inline awaitable<void> co_shared_mutex::lock()
{
	return m_native_handle.lock();
}

inline bool co_shared_mutex::try_lock()
{
	return m_native_handle.try_lock();
}

inline void co_shared_mutex::unlock()
{
	return m_native_handle.unlock();
}

awaitable<void> co_shared_mutex::lock_shared(concepts::schedulable auto &&exec)
{
	if( ++m_read_count == 1 )
		co_await m_native_handle.lock(exec);
	co_return ;
}

inline awaitable<void> co_shared_mutex::lock_shared()
{
	if( ++m_read_count == 1 )
		co_await m_native_handle.lock();
	co_return ;
}

inline bool co_shared_mutex::try_lock_shared()
{
	if( ++m_read_count == 1 )
		return m_native_handle.try_lock();
	return true;
}

inline void co_shared_mutex::unlock_shared()
{
	auto counter = m_read_count.load();
	if( counter == 0 )
		return m_native_handle.unlock();
	/*
		if( m_counter == counter )
		{
			m_counter = counter - 1;
			return true;
		}
		else
		{
			counter = m_counter;
			return false;
		}
	*/
	m_read_count.compare_exchange_weak(counter, counter - 1);
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_mutex::try_lock_for
(concepts::schedulable auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_native_handle.try_lock(exec, timeout);
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_mutex::try_lock_until
(concepts::schedulable auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_native_handle.try_lock(exec, timeout);
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_mutex::try_lock_for(const duration<Rep,Period> &timeout)
{
	return m_native_handle.try_lock(timeout);
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_mutex::try_lock_until(const time_point<Clock,Duration> &timeout)
{
	return m_native_handle.try_lock(timeout);
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_mutex::try_lock_shared_for
(concepts::schedulable auto &&exec, const duration<Rep,Period> &timeout)
{
	if( ++m_read_count == 1 )
		co_return co_await m_native_handle.try_lock(exec, timeout);
	co_return true;
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_mutex::try_lock_shared_until
(concepts::schedulable auto &&exec, const time_point<Clock,Duration> &timeout)
{
	if( ++m_read_count == 1 )
		co_return co_await m_native_handle.try_lock(exec, timeout);
	co_return true;
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_mutex::try_lock_shared_for(const duration<Rep,Period> &timeout)
{
	if( ++m_read_count == 1 )
		co_return co_await m_native_handle.try_lock(timeout);
	co_return true;
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_mutex::try_lock_shared_until(const time_point<Clock,Duration> &timeout)
{
	if( ++m_read_count == 1 )
		co_return co_await m_native_handle.try_lock(timeout);
	co_return true;
}

inline bool co_shared_mutex::is_locked() const noexcept
{
	return m_native_handle.is_locked();
}

inline co_shared_mutex::native_handle_t &co_shared_mutex::native_handle() noexcept
{
	return m_native_handle;
}

inline co_shared_lock::co_shared_lock(mutex_t &mutex) :
	m_mutex(&mutex)
{

}

inline co_shared_lock::~co_shared_lock() noexcept(noexcept(m_mutex->unlock_shared()))
{
	if( m_mutex )
		m_mutex->unlock_shared();
}

inline co_shared_lock::co_shared_lock(co_shared_lock &&other) noexcept :
	m_mutex(other.m_mutex)
{
	other.m_mutex = nullptr;
}

inline co_shared_lock &co_shared_lock::operator=(co_shared_lock &&other) noexcept
{
	if( this == &other )
		return *this;
	else if( m_mutex )
		m_mutex->unlock_shared();

	m_mutex = other.m_mutex;
	other.m_mutex = nullptr;
	return *this;
}

awaitable<void> co_shared_lock::lock_shared(concepts::schedulable auto &&exec)
{
	if( m_mutex )
		co_await m_mutex->lock_shared(exec);
	co_return ;
}

inline awaitable<void> co_shared_lock::lock_shared()
{
	co_return co_await lock_shared (
		co_await asio::this_coro::executor
	);
}

inline bool co_shared_lock::try_lock_shared()
{
	return m_mutex ? m_mutex->try_lock_shared() : true;
}

inline void co_shared_lock::unlock_shared()
{
	if( m_mutex )
		m_mutex->unlock_shared();
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_lock::try_lock_shared_for
(concepts::schedulable auto &&exec, const duration<Rep,Period> &timeout)
{
	co_return m_mutex ? co_await m_mutex->try_lock_shared_for(exec, timeout) : true;
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_lock::try_lock_shared_until
(concepts::schedulable auto &&exec, const time_point<Clock,Duration> &timeout)
{
	co_return m_mutex ? co_await m_mutex->try_lock_shared_until(exec, timeout) : true;
}

template<typename Rep, typename Period>
awaitable<bool> co_shared_lock::try_lock_shared_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_lock_shared_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<typename Clock, typename Duration>
awaitable<bool> co_shared_lock::try_lock_shared_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_lock_shared_until (
		co_await asio::this_coro::executor, timeout
	);
}

inline bool co_shared_lock::is_locked() const noexcept
{
	return m_mutex ? m_mutex->is_locked() : false;
}

inline co_shared_lock::mutex_t *co_shared_lock::mutex() noexcept
{
	return m_mutex;
}

} //namespace libgs


#endif //LIBGS_CORE_CORO_DETAIL_SHARED_MUTEX_H