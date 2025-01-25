
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

#ifndef LIBGS_CORE_CORO_DETAIL_MUTEX_H
#define LIBGS_CORE_CORO_DETAIL_MUTEX_H

#include <libgs/core/lock_free_queue.h>
#include <libgs/core/execution.h>

namespace libgs
{

class LIBGS_CORE_VAPI co_mutex::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	struct wake_up
	{
		using handler_t = async_work<>::handler_t;
		asio::any_io_executor exec;
		handler_t handler;

		void operator()()
		{
			dispatch(exec, [handler = std::make_shared<handler_t>(std::move(handler))]() mutable {
				std::move(*handler)();
			});
		}
	};
	using wake_up_ptr = std::shared_ptr<wake_up>;

public:
	impl() = default;
	~impl() noexcept(false)
	{
		if( not m_native_handle )
			return ;
		throw runtime_error (
			"libgs::co_mutex: Destruct a co_mutex that has not yet been unlocked."
		);
	}

public:
	native_handle_t m_native_handle {false};
	lock_free_queue<wake_up_ptr> m_wait_queue;
};

inline co_mutex::co_mutex() :
	m_impl(new impl())
{

}

inline co_mutex::~co_mutex() noexcept(false)
{
	delete m_impl;
}

awaitable<void> co_mutex::lock(concepts::schedulable auto &&exec)
{
	co_await async_work<>::handle(exec, [this, exec = get_executor_helper(exec)](auto &&wake_up)
	{
		if( try_lock() )
			std::move(wake_up)();
		else
		{
			m_impl->m_wait_queue.emplace (
				std::make_shared<impl::wake_up>(exec, std::move(wake_up))
			);
		}
	});
}

inline awaitable<void> co_mutex::lock()
{
	co_return co_await lock (
		co_await asio::this_coro::executor
	);
}

inline bool co_mutex::try_lock()
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
	return m_impl->m_native_handle.compare_exchange_weak(flag, true);
}

inline void co_mutex::unlock()
{
	auto wake_up = m_impl->m_wait_queue.dequeue();
	if( wake_up )
		std::move(**wake_up)();
	else
		m_impl->m_native_handle = false;
}

inline co_mutex::native_handle_t &co_mutex::native_handle() noexcept
{
	return m_impl->m_native_handle;
}

} //namespace libgs


#endif //LIBGS_CORE_CORO_DETAIL_MUTEX_H