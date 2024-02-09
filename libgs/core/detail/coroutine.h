
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

#ifndef LIBGS_CORE_DETAIL_COROUTINE_H
#define LIBGS_CORE_DETAIL_COROUTINE_H

#include <thread>

namespace libgs
{

template <typename Func, typename Exec>
auto co_spawn_detached(Func &&func, Exec &exec)
{
	return asio::co_spawn(exec, std::forward<Func>(func), asio::detached);
}

template <typename Exec, typename Func, typename...Args>
auto co_post(Exec &exec, Func &&func, Args&&...args) requires awaitable_function<Func,Args...>
{
	using return_type = decltype(func(std::forward<Args>(args)...));
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([&exec, func = std::forward<Func>(func)](auto handler, Args&&...args)
		{
		    auto work = asio::make_work_guard(handler);
		    asio::post(exec, [func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
		    {
				func(std::forward<Args>(args)...);
				asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					std::move(handler)();
				});
			});
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type&&)>
		([&exec, func = std::forward<Func>(func)](auto handler, Args&&...args)
		{
		    auto work = asio::make_work_guard(handler);
		    asio::post(exec, [func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
		    {
				auto res = func(std::forward<Args>(args)...);
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
		    });
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
}

template <typename Exec, typename Func, typename...Args>
auto co_dispatch(Exec &exec, Func &&func, Args&&...args) requires awaitable_function<Func,Args...>
{
	using return_type = decltype(func(std::forward<Args>(args)...));
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([&exec, func = std::forward<Func>(func)](auto handler, Args&&...args)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::dispatch(exec, [func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 func(std::forward<Args>(args)...);
				 asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					 std::move(handler)();
				 });
			 });
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type&&)>
		([&exec, func = std::forward<Func>(func)](auto handler, Args&&...args)
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(exec, [func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
			{
				auto res = func(std::forward<Args>(args)...);
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
			});
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
}

template <typename Func, typename...Args>
auto co_thread(Func &&func, Args&&...args) requires awaitable_function<Func,Args...>
{
	using return_type = decltype(func(std::forward<Args>(args)...));
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([func = std::forward<Func>(func)](auto handler, Args&&...args)
		{
			auto work = asio::make_work_guard(handler);
			std::thread([func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
			{
				func(std::forward<Args>(args)...);
				asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					std::move(handler)();
				});
			})
			.detach();
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type&&)>
		([func = std::forward<Func>(func)](auto handler, Args&&...args)
		{
			auto work = asio::make_work_guard(handler);
			std::thread([func = std::move(func), args..., handler = std::move(handler), work = std::move(work)]() mutable
			{
				auto res = func(std::forward<Args>(args)...);
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
			})
			.detach();
		},
		asio::use_awaitable, std::forward<Args>(args)...);
	}
}

template<typename Rep, typename Period, typename Exec>
awaitable<asio::error_code> co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, Exec &exec)
{
	asio::error_code error;
	asio::steady_timer timer(exec, rtime);
	co_await timer.async_wait(use_awaitable_e[error]);
	co_return error;
}

template<typename Clock, typename Duration, typename Exec>
awaitable<asio::error_code> co_sleep_until(const std::chrono::time_point<Clock,Duration> &atime, Exec &exec)
{
	asio::error_code error;
	asio::steady_timer timer(exec, atime);
	co_await timer.async_wait(use_awaitable_e[error]);
	co_return error;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_COROUTINE_H
