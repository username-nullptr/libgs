
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

template <concepts::schedulable Exec>
auto co_spawn(concepts::awaitable_function auto &&func, Exec &exec)
{
	using Func = std::decay_t<decltype(func)>;
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::forward<Func>(func), asio::use_awaitable);
	else
		return asio::co_spawn(exec, std::forward<Func>(func), asio::use_awaitable);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn(awaitable<T> &&a, Exec &exec)
{
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::move(a), asio::use_awaitable);
	else
		return asio::co_spawn(exec, std::move(a), asio::use_awaitable);
}

template <concepts::schedulable Exec>
auto co_spawn_detached(concepts::awaitable_function auto &&func, Exec &exec)
{
	using Func = std::decay_t<decltype(func)>;
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::forward<Func>(func), asio::detached);
	else
		return asio::co_spawn(exec, std::forward<Func>(func), asio::detached);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn_detached(awaitable<T> &&a, Exec &exec)
{
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::move(a), asio::detached);
	else
		return asio::co_spawn(exec, std::move(a), asio::detached);
}

template <concepts::schedulable Exec>
auto co_spawn_future(concepts::awaitable_function auto &&func, Exec &exec)
{
	using Func = std::decay_t<decltype(func)>;
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::forward<Func>(func), asio::use_future);
	else
		return asio::co_spawn(exec, std::forward<Func>(func), asio::use_future);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn_future(awaitable<T> &&a, Exec &exec)
{
	if constexpr( is_execution_context_v<Exec> )
		return asio::co_spawn(exec.get_executor(), std::move(a), asio::use_future);
	else
		return asio::co_spawn(exec, std::move(a), asio::use_future);
}

template <concepts::schedulable Exec, typename Func>
auto co_post(Exec &exec, Func &&func) requires concepts::callable<Func>
{
	using return_type = decltype(func());
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([&exec, func = std::forward<Func>(func)](auto handler)
		{
		    auto work = asio::make_work_guard(handler);
		    asio::post(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
		    {
				func();
				asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					std::move(handler)();
				});
			});
		},
		asio::use_awaitable);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type)>
		([&exec, func = std::forward<Func>(func)](auto handler)
		{
		    auto work = asio::make_work_guard(handler);
		    asio::post(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
		    {
				auto res = func();
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
		    });
		},
		asio::use_awaitable);
	}
}

template <concepts::schedulable Exec, typename Func>
auto co_dispatch(Exec &exec, Func &&func) requires concepts::callable<Func>
{
	using return_type = decltype(func());
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([&exec, func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::dispatch(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 func();
				 asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					 std::move(handler)();
				 });
			 });
		},
		asio::use_awaitable);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type)>
		([&exec, func = std::forward<Func>(func)](auto handler)
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			{
				auto res = func();
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
			});
		},
		asio::use_awaitable);
	}
}

template <typename Func>
auto co_thread(Func &&func) requires concepts::callable<Func>
{
	using return_type = decltype(func());
	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([func = std::forward<Func>(func)](auto handler)
		{
			auto work = asio::make_work_guard(handler);
			std::thread([func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			{
				func();
				asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					std::move(handler)();
				});
			})
			.detach();
		},
		asio::use_awaitable);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_type)>
		([func = std::forward<Func>(func)](auto handler)
		{
			auto work = asio::make_work_guard(handler);
			std::thread([func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			{
				auto res = func();
				asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					std::move(handler)(std::move(res));
				});
			})
			.detach();
		},
		asio::use_awaitable);
	}
}

template<typename Rep, typename Period, concepts::schedulable Exec>
awaitable<error_code> co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, Exec &exec)
{
	error_code error;
	asio::steady_timer timer(exec, rtime);
	co_await timer.async_wait(use_awaitable|error);
	co_return error;
}

template<typename Clock, typename Duration, concepts::schedulable Exec>
awaitable<error_code> co_sleep_until(const std::chrono::time_point<Clock,Duration> &atime, Exec &exec)
{
	error_code error;
	asio::steady_timer timer(exec, atime);
	co_await timer.async_wait(use_awaitable|error);
	co_return error;
}

template <typename T>
awaitable<T> co_wait(const std::future<T> &future)
{
	co_return co_await co_thread([future = &future] {
		return remove_const(future)->get();
	});
}

inline awaitable<void> co_wait(const asio::thread_pool &pool)
{
	co_return co_await co_thread([pool = &pool] {
		return remove_const(pool)->wait();
	});
}

inline awaitable<void> co_wait(const std::thread &thread)
{
	co_return co_await co_thread([thread = &thread] {
		return remove_const(thread)->join();
	});
}

template <concepts::schedulable Exec>
awaitable<void> co_to_exec(Exec &exec)
{
	return asio::async_initiate<decltype(asio::use_awaitable), void()>([&exec](auto handler)
	{
	    auto work = asio::make_work_guard(handler);
	    asio::post(exec, [handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)();
		});
	},
	asio::use_awaitable);
}

inline awaitable<void> co_to_thread()
{
	return asio::async_initiate<decltype(asio::use_awaitable), void()>([](auto handler)
	{
	    auto work = asio::make_work_guard(handler);
		std::thread([handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)();
		}).detach();
	},
	asio::use_awaitable);
}

template <concepts::co_token Token>
bool check_error(Token &token, const error_code &error, const char *func)
	requires (not std::is_const_v<Token>)
{
	if( not error )
		return true;
	if constexpr( is_use_awaitable_v<Token> or is_cancellation_slot_binder_v<Token> )
		throw func ? system_error(error, func) : system_error(error);
	else if constexpr( is_redirect_error_v<Token> or is_redirect_error_cancellation_slot_binder_v<Token> )
		token.ec_ = error;
	else if constexpr( is_cancellation_slot_binder_redirect_error_v<Token> )
		token.get().ec_ = error;
	else
		static_assert(false, "aaaaaaaaaaaaaaaaaaaaaaaa");
	return false;
}

#ifdef LIBGS_USING_BOOST_ASIO

template <concepts::schedulable Exec, concepts::execution YCExec, typename Func>
auto co_post(Exec &exec, basic_yield_context<YCExec> yc, Func &&func) requires concepts::callable<Func>
{
	using yield_context = basic_yield_context<YCExec>;
	using return_type = decltype(func());

	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([&exec, func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::post(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 func();
				 asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					 std::move(handler)();
				 });
			 });
		 },
		 yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_type)>
		([&exec, func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::post(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 auto res = func();
				 asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					 std::move(handler)(std::move(res));
				 });
			 });
		 },
		 yc);
	}
}

template <concepts::schedulable Exec, concepts::execution YCExec, typename Func>
auto co_dispatch(Exec &exec, basic_yield_context<YCExec> yc, Func &&func) requires concepts::callable<Func>
{
	using yield_context = basic_yield_context<YCExec>;
	using return_type = decltype(func());

	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([&exec, func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::dispatch(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 func();
				 asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					 std::move(handler)();
				 });
			 });
		 },
		 yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_type)>
		([&exec, func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 asio::dispatch(exec, [func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 auto res = func();
				 asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					 std::move(handler)(std::move(res));
				 });
			 });
		 },
		 yc);
	}
}

template <concepts::execution YCExec, typename Func>
auto co_thread(basic_yield_context<YCExec> yc, Func &&func) requires concepts::callable<Func>
{
	using yield_context = basic_yield_context<YCExec>;
	using return_type = decltype(func());

	if constexpr( std::is_same_v<return_type, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 std::thread([func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 func();
				 asio::dispatch(work.get_executor(), [handler = std::move(handler)]() mutable {
					 std::move(handler)();
				 });
			 })
			 .detach();
		 },
		 yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_type)>
		([func = std::forward<Func>(func)](auto handler)
		 {
			 auto work = asio::make_work_guard(handler);
			 std::thread([func = std::move(func), handler = std::move(handler), work = std::move(work)]() mutable
			 {
				 auto res = func();
				 asio::dispatch(work.get_executor(), [res = std::move(res), handler = std::move(handler)]() mutable {
					 std::move(handler)(std::move(res));
				 });
			 })
			 .detach();
		 },
		 yc);
	}
}

template<typename Rep, typename Period, concepts::execution YCExec, concepts::schedulable Exec>
error_code co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, basic_yield_context<Exec> yc, Exec &exec)
{
	error_code error;
	asio::steady_timer timer(exec, rtime);
	timer.async_wait(yc[error]);
	return error;
}

template<typename Clock, typename Duration, concepts::execution YCExec, concepts::schedulable Exec>
error_code co_sleep_until(const std::chrono::time_point<Clock,Duration> &atime, basic_yield_context<Exec> yc, Exec &exec)
{
	error_code error;
	asio::steady_timer timer(exec, atime);
	timer.async_wait(yc[error]);
	return error;
}

template <typename T, concepts::execution YCExec>
T co_wait(basic_yield_context<YCExec> yc, const std::future<T> &future)
{
	return co_thread(yc, [future = &future] {
		return remove_const(future)->get();
	});
}

template <concepts::execution YCExec>
void co_wait(basic_yield_context<YCExec> yc, const asio::thread_pool &pool)
{
	co_thread(yc, [pool = &pool] {
		return remove_const(pool)->wait();
	});
}

template <concepts::execution YCExec>
void co_wait(basic_yield_context<YCExec> yc, const std::thread &thread)
{
	co_thread(yc, [thread = &thread] {
		return remove_const(thread)->join();
	});
}

template <concepts::execution YCExec, concepts::schedulable Exec>
awaitable<void> co_to_exec(basic_yield_context<YCExec> yc, Exec &exec)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>([&exec](auto handler)
	{
		auto work = asio::make_work_guard(handler);
		asio::post(exec, [handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)();
		});
	},
	yc);
}

template <concepts::execution YCExec>
void co_to_thread(basic_yield_context<YCExec> yc)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>([](auto handler)
	{
		auto work = asio::make_work_guard(handler);
		std::thread([handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)();
		}).detach();
	},
	yc);
}

template <concepts::execution Exec>
bool check_error(basic_yield_context<Exec> &yc, const error_code &error, const char *func)
{
	if( not error )
		return true;
	else if( not yc.ec_ )
		throw func ? system_error(error, func) : system_error(error);
	*yc.ec_ = error;
	return false;
}

#endif //LIBGS_USING_BOOST_ASIO

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_COROUTINE_H
