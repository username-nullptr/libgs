
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

#ifndef LIBGS_CORE_CORO_DETAIL_UTILITIES_H
#define LIBGS_CORE_CORO_DETAIL_UTILITIES_H

#include <libgs/http/cxx/file_opt_token.h>
#include <thread>

namespace libgs
{

template <typename Rep, typename Period, concepts::co_sleep_opt_token Token>
auto co_sleep_for(concepts::schedulable auto &&exec, const duration<Rep,Period> &rtime, Token &&token)
{
	return sleep_for(std::forward<decltype(exec)>(exec), rtime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::co_sleep_opt_token Token>
auto co_sleep_for(const duration<Rep,Period> &rtime, Token &&token)
{
	return sleep_for(rtime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::co_sleep_opt_token Token>
auto co_sleep_until(concepts::schedulable auto &&exec, const time_point<Rep,Period> &atime, Token &&token)
{
	return sleep_for(std::forward<decltype(exec)>(exec), atime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::co_sleep_opt_token Token>
auto co_sleep_until(const time_point<Rep,Period> &atime, Token &&token)
{
	return sleep_for(atime, std::forward<Token>(token));
}

template <typename T>
awaitable<T> co_wait(const std::future<T> &future)
{
	auto exec = co_await asio::this_coro::executor;
	co_return co_await dispatch(exec, [&future]() mutable -> awaitable<T>
	{
		auto res = co_await local_dispatch([&future] {
			return remove_const(future).get();
		}, use_awaitable);

		if constexpr( std::is_void_v<T> )
			co_return ;
		else
			co_return res.first;
	},
	use_awaitable);
}

inline awaitable<void> co_wait(const asio::thread_pool &pool)
{
	auto exec = co_await asio::this_coro::executor;
	co_return co_await dispatch(exec, [&pool]() mutable -> awaitable<void>
	{
		co_await local_dispatch([&pool] {
			return remove_const(pool).wait();
		}, use_awaitable);
		co_return ;
	},
	use_awaitable);
}

inline awaitable<void> co_wait(const std::thread &thread)
{
	auto exec = co_await asio::this_coro::executor;
	co_return co_await dispatch(exec, [&thread]() mutable -> awaitable<void>
	{
		co_await local_dispatch([&thread] {
			return remove_const(thread).join();
		}, use_awaitable);
		co_return ;
	},
	use_awaitable);
}

template <concepts::schedulable Exec>
awaitable<asio::any_io_executor> co_to_exec(Exec &&exec)
{
	auto curr_exec = co_await asio::this_coro::executor;
	co_return co_await asio::async_initiate<decltype(asio::use_awaitable), void(asio::any_io_executor)>
	([curr_exec = std::move(curr_exec), exec = get_executor_helper(exec)](auto handler)
	{
	    auto work = asio::make_work_guard(handler);
	    asio::post(exec, [
			handler = std::move(handler), work = std::move(work), prev_exec = std::move(curr_exec)
	    ]() mutable
	    {
	    	LIBGS_UNUSED(work);
			std::move(handler)(std::move(prev_exec));
		});
	},
	asio::use_awaitable);
}

inline awaitable<asio::any_io_executor> co_to_thread()
{
	auto curr_exec = co_await asio::this_coro::executor;
	co_return co_await asio::async_initiate<decltype(asio::use_awaitable), void(asio::any_io_executor)>
	([curr_exec = std::move(curr_exec)](auto handler)
	{
	    auto work = asio::make_work_guard(handler);
		std::thread([
			handler = std::move(handler), work = std::move(work), prev_exec = std::move(curr_exec)
		]() mutable
		{
	    	LIBGS_UNUSED(work);
			std::move(handler)(std::move(prev_exec));
		}).detach();
	},
	asio::use_awaitable);
}

template <concepts::any_async_tf_opt_token Token>
bool check_error(Token &token, const error_code &error, const char *message)
	requires (not std::is_const_v<Token>)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_use_awaitable_v<token_t> )
	{
		if( not error )
			return true;
		throw message ? system_error(error, message) : system_error(error);
	}
	else if constexpr( is_redirect_error_v<token_t> )
	{
		token.ec_ = error;
		return false;
	}
	else if constexpr( is_cancellation_slot_binder_v<token_t> )
		return check_error(token.get(), error, message);

	else if constexpr( is_redirect_time_v<token_t> )
		return check_error(token.token, error, message);
	else
	{
		static_assert(false, "Unsupported token type.");
		return false;
	}
}

#ifdef LIBGS_USING_BOOST_ASIO

template <concepts::execution YCExec>
auto co_post(concepts::schedulable auto &&exec, basic_yield_context<YCExec> yc, concepts::callable auto &&func)
{
	using yield_context = basic_yield_context<YCExec>;
	using function_t = std::decay_t<decltype(func)>;
	using executor_t = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>([
			exec = get_executor_helper(std::forward<executor_t>(exec)), func = std::forward<function_t>(func)
		](auto handler)
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
		([exec = get_executor_helper(exec), func = std::forward<function_t>(func)](auto handler)
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
	using function_t = std::decay_t<decltype(func)>;
	using executor_t = std::decay_t<decltype(exec)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>([
			exec = get_executor_helper(std::forward<executor_t>(exec)), func = std::forward<function_t>(func)
		](auto handler)
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
		([exec = get_executor_helper(exec), func = std::forward<function_t>(func)](auto handler)
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
	using function_t = std::decay_t<decltype(func)>;
	using return_t = decltype(func());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([func = std::forward<function_t>(func)](auto handler)
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
		([func = std::forward<function_t>(func)](auto handler)
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
asio::any_io_executor co_to_exec(basic_yield_context<YCExec> yc, Exec &&exec)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>
	([exec = get_executor_helper(std::forward<Exec>(exec))](auto handler)
	{
		auto work = asio::make_work_guard(handler);
		asio::post(exec, [handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)(work.get_executor());
		});
	},
	yc);
}

template <concepts::execution YCExec>
asio::any_io_executor co_to_thread(basic_yield_context<YCExec> yc)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>([](auto handler)
	{
		auto work = asio::make_work_guard(handler);
		std::thread([handler = std::move(handler), work = std::move(work)]() mutable {
			std::move(handler)(work.get_executor());
		}).detach();
	},
	yc);
}

template <concepts::execution Exec>
bool check_error(basic_yield_context<Exec> &yc, const error_code &error, const char *message)
{
	if( not error )
		return true;
	else if( not yc.ec_ )
		throw func ? system_error(error, message) : system_error(error);
	*yc.ec_ = error;
	return false;
}

#endif //LIBGS_USING_BOOST_ASIO

} //namespace libgs


#endif //LIBGS_CORO_DETAIL_UTILITIES_H
