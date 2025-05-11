
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

#ifndef LIBGS_CORO_DETAIL_SEMAPHORE_H
#define LIBGS_CORO_DETAIL_SEMAPHORE_H

#include <libgs/core/lock_free_queue.h>
#include <libgs/coro/detail/wake_up.h>

#include <semaphore>

namespace libgs::coro
{

template<size_t Max>
class LIBGS_CORE_TAPI basic_semaphore<Max>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using wake_up_t = detail::lock_wake_up;

	explicit impl(size_t initial_count) :
		m_counter(initial_count)
	{
		if( initial_count > max_v )
		{
			throw std::invalid_argument (
				"libgs::basic_semaphore: Initial count is greater than max value."
			);
		}
	}

	~impl()
	{
#if 0
		auto wake_up = m_wait_queue.dequeue();
		if( not wake_up )
			return ;
		throw runtime_error (
			"libgs::basic_semaphore: Destruct a basic_semaphore with unreleased resources."
		);
#else
		while( m_counter < max_v )
			release_one();
#endif
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
		return m_counter.compare_exchange_strong(counter, counter - 1);
	}

	[[nodiscard]] awaitable<bool> try_acquire_x
	(concepts::sched auto &&exec, const auto &timeout)
	{
		if( try_acquire() )
			co_return true;

		co_return co_await async_work<bool>::handle(exec,
		[this, timeout, exec = get_executor_helper(exec)](auto &&wake_up) mutable
		{
			auto wake_up_ptr = std::make_shared<wake_up_t>(exec, std::move(wake_up));
			m_wait_queue.emplace(wake_up_ptr);
			wake_up_ptr->start_timer(timeout);
		});
	}

	void release_one()
	{
		for(;;)
		{
			auto wake_up = m_wait_queue.dequeue();
			if( wake_up )
			{
				if( not std::move(**wake_up)(true) )
					continue;
			}
			else
				m_counter.fetch_add(1);
			break;
		}
	}

public:
	std::atomic_size_t m_counter = 0;
	lock_free_queue<detail::lock_wake_up_ptr> m_wait_queue;
};

template<size_t Max>
basic_semaphore<Max>::basic_semaphore(size_t initial_count) :
	m_impl(new impl(initial_count))
{

}

template<size_t Max>
basic_semaphore<Max>::~basic_semaphore()
{
	delete m_impl;
}

template<size_t Max>
awaitable<void> basic_semaphore<Max>::acquire(concepts::sched auto &&exec)
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
awaitable<void> basic_semaphore<Max>::acquire()
{
	co_return co_await acquire (
		co_await asio::this_coro::executor
	);
}

template<size_t Max>
bool basic_semaphore<Max>::try_acquire()
{
	return m_impl->try_acquire();
}

template<size_t Max>
size_t basic_semaphore<Max>::release(size_t n) requires (max_v > 1)
{
	if( n == 0 or n > max_v - m_impl->m_counter )
	{
		throw std::invalid_argument (
			"libgs::basic_semaphore: Invalid release count."
		);
	}
	while( n-- )
		m_impl->release_one();
	return count();
}

template<size_t Max>
size_t basic_semaphore<Max>::release() requires (max_v == 1)
{
	if( m_impl->m_counter == 1 )
	{
		throw std::runtime_error (
			"libgs::basic_semaphore: Release a binary_semaphore with max count 1 more than once."
		);
	}
	m_impl->release_one();
	return count();
}

template<size_t Max>
template<typename Rep, typename Period>
awaitable<bool> basic_semaphore<Max>::try_acquire_for
(concepts::sched auto &&exec, const duration<Rep,Period> &timeout)
{
	return m_impl->try_acquire_x(exec,
		std::chrono::duration_cast<asio::steady_timer::duration>(timeout)
	);
}

template<size_t Max>
template<typename Clock, typename Duration>
awaitable<bool> basic_semaphore<Max>::try_acquire_until
(concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout)
{
	return m_impl->try_acquire_x(exec,
		std::chrono::time_point_cast<asio::steady_timer::time_point>(timeout)
	);
}

template<size_t Max>
template<typename Rep, typename Period>
awaitable<bool> basic_semaphore<Max>::try_acquire_for(const duration<Rep,Period> &timeout)
{
	co_return co_await try_acquire_for (
		co_await asio::this_coro::executor, timeout
	);
}

template<size_t Max>
template<typename Clock, typename Duration>
awaitable<bool> basic_semaphore<Max>::try_acquire_until(const time_point<Clock,Duration> &timeout)
{
	co_return co_await try_acquire_until (
		co_await asio::this_coro::executor, timeout
	);
}

template<size_t Max>
consteval size_t basic_semaphore<Max>::max() const noexcept
{
	return max_v;
}

template<size_t Max>
size_t basic_semaphore<Max>::count() const noexcept
{
	return m_impl->m_counter;
}

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_SEMAPHORE_H