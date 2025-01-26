
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

#ifndef LIBGS_CORE_CORO_DETAIL_SEMAPHORE_H
#define LIBGS_CORE_CORO_DETAIL_SEMAPHORE_H

#include <libgs/core/coro/detail/wake_up.h>
#include <libgs/core/lock_free_queue.h>

#include <semaphore>

namespace libgs
{

template<size_t Max>
class LIBGS_CORE_TAPI co_semaphore<Max>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using wake_up_t = detail::co_lock_wake_up;

	explicit impl(size_t initial_count) :
		m_counter(initial_count)
	{
		if( initial_count > max_v )
		{
			throw std::invalid_argument (
				"libgs::co_semaphore: Initial count is greater than max value."
			);
		}
	}

	~impl() noexcept(false)
	{
		if( m_counter == max_v )
			return ;
		throw runtime_error (
			"libgs::co_semaphore: Destruct a co_semaphore with unreleased resources."
		);
	}

public:
	[[nodiscard]] bool try_acquire()
	{
		auto counter = m_counter.load();
		if( counter == 0 )
			return false;
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
		return m_counter.compare_exchange_weak(counter, counter - 1);
	}

	[[nodiscard]] awaitable<bool> try_acquire_x
	(concepts::schedulable auto &&exec, const auto &timeout)
	{
		if( try_acquire() )
			co_return true;

		co_return co_await async_work<bool>::handle(exec,
		[this, timeout, exec = get_executor_helper(exec)](auto &&wake_up) mutable
		{
			auto wake_up_ptr = std::make_shared<wake_up_t>(exec, std::move(wake_up));
			m_wait_queue.emplace(wake_up_ptr);

			auto timer = asio::steady_timer(exec, timeout);
			timer.async_wait([this, wake_up_ptr = std::move(wake_up_ptr)]() mutable {
				std::move(**wake_up_ptr)(false);
			});
		});
	}

public:
	std::atomic_size_t m_counter = 0;
	lock_free_queue<detail::co_lock_wake_up_ptr> m_wait_queue;
};

template<size_t Max>
co_semaphore<Max>::co_semaphore(size_t initial_count) :
	m_impl(new impl(initial_count))
{

}

template<size_t Max>
co_semaphore<Max>::~co_semaphore() noexcept(false)
{
	delete m_impl;
}

template<size_t Max>
awaitable<void> co_semaphore<Max>::acquire(concepts::schedulable auto &&exec)
{
	if( try_acquire() )
		co_return ;

	co_await async_work<bool>::handle(exec,
	[this, exec = get_executor_helper(exec)](async_work<bool>::handler_t wake_up) mutable
	{
		m_impl->m_wait_queue.emplace (
			std::make_shared<typename impl::wake_up_t>(exec, std::move(wake_up))
		);
	});
	co_return ;
}

template<size_t Max>
awaitable<void> co_semaphore<Max>::acquire()
{
	co_return co_await acquire (
		co_await asio::this_coro::executor
	);
}

template<size_t Max>
bool co_semaphore<Max>::try_acquire()
{
	return m_impl->try_acquire();
}

template<size_t Max>
void co_semaphore<Max>::release(size_t n) requires (max_v > 1)
{
	if( n == 0 or n > max_v - m_impl->m_counter )
	{
		throw std::invalid_argument (
			"libgs::co_semaphore: Invalid release count."
		);
	}
	while( n-- )
	{
		auto wake_up = m_impl->m_wait_queue.dequeue();
		if( wake_up )
			std::move(**wake_up)(true);
		else
			m_impl->m_counter.fetch_add(1);
	}
}

template<size_t Max>
void co_semaphore<Max>::release() requires (max_v == 1)
{
	if( m_impl->m_counter == 1 )
	{
		throw std::runtime_error (
			"libgs::co_semaphore: Release a co_binary_semaphore with max count 1 more than once."
		);
	}
	auto wake_up = m_impl->m_wait_queue.dequeue();
	if( wake_up )
		std::move(**wake_up)(true);
	else
		m_impl->m_counter.fetch_add(1);
}

template<size_t Max>
template<typename Rep, typename Period>
awaitable<bool> co_semaphore<Max>::try_acquire_for
(concepts::schedulable auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_impl->try_acquire_x(exec,
		std::chrono::duration_cast<asio::steady_timer::duration>(timeout)
	);
}

template<size_t Max>
template<typename Clock, typename Duration>
awaitable<bool> co_semaphore<Max>::try_acquire_until
(concepts::schedulable auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_impl->try_acquire_x(exec,
		std::chrono::time_point_cast<asio::steady_timer::time_point>(timeout)
	);
}

template<size_t Max>
template<typename Rep, typename Period>
awaitable<bool> co_semaphore<Max>::try_acquire_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_acquire_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<size_t Max>
template<typename Clock, typename Duration>
awaitable<bool> co_semaphore<Max>::try_acquire_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_acquire_until (
		co_await asio::this_coro::executor, timeout
	);
}

template<size_t Max>
consteval size_t co_semaphore<Max>::max() const noexcept
{
	return max_v;
}

} //namespace libgs


#endif //LIBGS_CORE_CORO_DETAIL_SEMAPHORE_H