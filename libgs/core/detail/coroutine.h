
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
#include <iostream>

namespace libgs
{

template <concepts::schedulable Exec>
auto co_spawn(concepts::awaitable_function auto &&func, Exec &&exec)
{
	return asio::co_spawn (
		std::forward<Exec>(exec),
		std::forward<std::decay_t<decltype(func)>>(func),
		asio::use_awaitable
	);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn(awaitable<T> &&a, Exec &&exec)
{
	return asio::co_spawn (
		get_executor_helper(std::forward<Exec>(exec)),
		std::move(a), asio::use_awaitable
	);
}

template <concepts::schedulable Exec>
auto co_spawn_detached(concepts::awaitable_function auto &&func, Exec &&exec)
{
	return asio::co_spawn (
		get_executor_helper(std::forward<Exec>(exec)),
		std::forward<std::decay_t<decltype(func)>>(func),
		asio::detached
	);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn_detached(awaitable<T> &&a, Exec &&exec)
{
	return asio::co_spawn (
		get_executor_helper(std::forward<Exec>(exec)),
		std::move(a), asio::detached
	);
}

template <concepts::schedulable Exec>
auto co_spawn_future(concepts::awaitable_function auto &&func, Exec &&exec)
{
	return asio::co_spawn (
		get_executor_helper(std::forward<Exec>(exec)),
		std::forward<std::decay_t<decltype(func)>>(func),
		asio::use_future
	);
}

template <typename T, concepts::schedulable Exec>
auto co_spawn_future(awaitable<T> &&a, Exec &&exec)
{
	return asio::co_spawn (
		get_executor_helper(std::forward<Exec>(exec)),
		std::move(a), asio::use_future
	);
}

template <typename T>
auto co_task(std::function<void(awaitable_wake_up<T>&&)> wake_up)
{
	if constexpr( std::is_same_v<T,void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([wake_up = std::move(wake_up)](auto handler)
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(work.get_executor(),
			[wake_up = std::move(wake_up), handler = std::move(handler)]() mutable {
				wake_up(std::move(handler));
			});
		},
		asio::use_awaitable);
	}
	else
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void(T)>
		([wake_up = std::move(wake_up)](auto handler)
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(work.get_executor(),
			[wake_up = std::move(wake_up), handler = std::move(handler)]() mutable {
				wake_up(std::move(handler));
			});
		},
		asio::use_awaitable);
	}
}

auto co_post(concepts::schedulable auto &&exec, concepts::callable auto &&func)
{
	using Func = std::decay_t<decltype(func)>;
	using Exec = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(func)](auto handler)
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
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_t)>
		([exec = get_executor_helper(exec), func = std::forward<Func>(func)](auto handler)
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

auto co_dispatch(concepts::schedulable auto &&exec, concepts::callable auto &&func)
{
	using Func = std::decay_t<decltype(func)>;
	using Exec = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<decltype(asio::use_awaitable), void()>
		([exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(func)](auto handler)
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
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_t)>
		([exec = get_executor_helper(exec), func = std::forward<Func>(func)](auto handler)
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

auto co_thread(concepts::callable auto &&func)
{
	using Func = std::decay_t<decltype(func)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
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
		return asio::async_initiate<decltype(asio::use_awaitable), void(return_t)>
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

namespace detail
{

template <typename Exec>
awaitable<error_code> co_sleep_x(const auto &stdtime, Exec &&exec)
{
	error_code error;
	asio::steady_timer timer(std::forward<Exec>(exec), stdtime);
	co_await timer.async_wait(use_awaitable|error);
	co_return error;
}

} //namespace detail

template <typename Rep, typename Period, concepts::schedulable Exec>
awaitable<error_code> co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, Exec &&exec)
{
	return detail::co_sleep_x(rtime, std::forward<Exec>(exec));
}

template <typename Rep, typename Period, concepts::schedulable Exec>
awaitable<error_code> co_sleep_until(const std::chrono::time_point<Rep,Period> &atime, Exec &&exec)
{
	return detail::co_sleep_x(atime, std::forward<Exec>(exec));
}

template <typename T>
awaitable<T> co_wait(const std::future<T> &future)
{
	return co_thread([&future] {
		return remove_const(future).get();
	});
}

inline awaitable<void> co_wait(const asio::thread_pool &pool)
{
	return co_thread([&pool] {
		return remove_const(pool).wait();
	});
}

inline awaitable<void> co_wait(const std::thread &thread)
{
	return co_thread([&thread] {
		return remove_const(thread).join();
	});
}

template <concepts::schedulable Exec>
awaitable<void> co_to_exec(Exec &&exec)
{
	return asio::async_initiate<decltype(asio::use_awaitable), void()>
	([exec = get_executor_helper(exec)](auto handler)
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
		static_assert(false, "Unsupported token type.");
	return false;
}

#ifdef LIBGS_USING_BOOST_ASIO

template <concepts::execution YCExec>
auto co_post(concepts::schedulable auto &&exec, basic_yield_context<YCExec> yc, concepts::callable auto &&func)
{
	using yield_context = basic_yield_context<YCExec>;
	using Func = std::decay_t<decltype(func)>;
	using Exec = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(func)](auto handler)
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
		return asio::async_initiate<yield_context, void(return_t)>
		([exec = get_executor_helper(exec), func = std::forward<Func>(func)](auto handler)
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

template <concepts::execution YCExec>
auto co_dispatch(concepts::schedulable auto &&exec, basic_yield_context<YCExec> yc, concepts::callable auto &&func)
{
	using yield_context = basic_yield_context<YCExec>;
	using Func = std::decay_t<decltype(func)>;
	using Exec = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(func)](auto handler)
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
		return asio::async_initiate<yield_context, void(return_t)>
		([exec = get_executor_helper(exec), func = std::forward<Func>(func)](auto handler)
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

template <concepts::execution YCExec>
auto co_thread(basic_yield_context<YCExec> yc, concepts::callable auto &&func)
{
	using yield_context = basic_yield_context<YCExec>;
	using Func = std::decay_t<decltype(func)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
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
		return asio::async_initiate<yield_context, void(return_t)>
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

namespace detail
{

template<concepts::execution YCExec, typename Exec>
error_code co_sleep_x(const auto &stdtime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	error_code error;
	asio::steady_timer timer(std::forward<Exec>(exec), stdtime);
	timer.async_wait(yc[error]);
	return error;
}

} //namespace detail

template<typename Rep, typename Period, concepts::execution YCExec, concepts::schedulable Exec>
error_code co_sleep_for
(const std::chrono::duration<Rep,Period> &rtime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	return detail::co_sleep_x(rtime, yc, std::forward<Exec>(exec));
}

template<typename Clock, typename Duration, concepts::execution YCExec, concepts::schedulable Exec>
error_code co_sleep_until
(const std::chrono::time_point<Clock,Duration> &atime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	return detail::co_sleep_x(atime, yc, std::forward<Exec>(exec));
}

template <typename T, concepts::execution YCExec>
T co_wait(basic_yield_context<YCExec> yc, const std::future<T> &future)
{
	return co_thread(yc, [&future] {
		return remove_const(future).get();
	});
}

template <concepts::execution YCExec>
void co_wait(basic_yield_context<YCExec> yc, const asio::thread_pool &pool)
{
	co_thread(yc, [&pool] {
		return remove_const(pool).wait();
	});
}

template <concepts::execution YCExec>
void co_wait(basic_yield_context<YCExec> yc, const std::thread &thread)
{
	co_thread(yc, [&thread] {
		return remove_const(thread).join();
	});
}

template <concepts::execution YCExec, concepts::schedulable Exec>
awaitable<void> co_to_exec(basic_yield_context<YCExec> yc, Exec &&exec)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>
	([exec = get_executor_helper(std::forward<Exec>(exec))](auto handler)
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
