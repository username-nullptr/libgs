// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_DETAIL_UTILS_H
#define LIBGS_CORO_DETAIL_UTILS_H

#include <libgs/core/execution.h>
#include <thread>

namespace libgs::coro
{

template <typename Rep, typename Period, concepts::sleep_opt_token Token>
auto sleep_for(concepts::sched auto &&exec, duration<Rep,Period> rtime, Token &&token)
{
	return libgs::sleep_for(std::forward<decltype(exec)>(exec), rtime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::sleep_opt_token Token>
auto sleep_for(duration<Rep,Period> rtime, Token &&token)
{
	return libgs::sleep_for(rtime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::sleep_opt_token Token>
auto sleep_until(concepts::sched auto &&exec, time_point<Rep,Period> atime, Token &&token)
{
	return libgs::sleep_until(std::forward<decltype(exec)>(exec), atime, std::forward<Token>(token));
}

template <typename Rep, typename Period, concepts::sleep_opt_token Token>
auto sleep_until(time_point<Rep,Period> atime, Token &&token)
{
	return libgs::sleep_until(atime, std::forward<Token>(token));
}

template <typename T>
awaitable<T> wait(const std::future<T> &future)
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

template <concepts::sched Exec>
awaitable<asio::any_io_executor> goto_exec(Exec &&executor_arg)
{
	auto current_exec = co_await asio::this_coro::executor;
	co_return co_await asio::async_initiate<decltype(asio::use_awaitable), void(asio::any_io_executor)>
	([previous_exec = std::move(current_exec), target_exec = get_executor_helper(executor_arg)](auto completion_handler)
	{
		auto work_guard = asio::make_work_guard(completion_handler);
		asio::post(target_exec, [
			posted_handler = std::move(completion_handler), guard = std::move(work_guard),
			return_exec = std::move(previous_exec)
		]() mutable
		{
			LIBGS_UNUSED(guard);
			std::move(posted_handler)(std::move(return_exec));
		});
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
		if( error )
		{
			system_error::loc_throw(error,
				with_location(message ? message : "")
			);
		}
		return true;
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

template <concepts::exec YCExec>
auto co_post
(concepts::sched auto &&executor_arg, basic_yield_context<YCExec> yc, concepts::callable auto &&work_fn)
{
	using yield_context = basic_yield_context<YCExec>;
	using function_t = std::decay_t<decltype(work_fn)>;
	using executor_t = std::decay_t<decltype(executor_arg)>;
	using return_t = decltype(work_fn());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>([
			exec = get_executor_helper(std::forward<executor_t>(executor_arg)),
			func = std::forward<function_t>(work_fn)
		](auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			asio::post(exec, [
				posted_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				posted_func();
				asio::dispatch(work.get_executor(), [completion = std::move(handler)]() mutable {
					std::move(completion)();
				});
			});
		},
		yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_t)>
		([exec = get_executor_helper(executor_arg), func = std::forward<function_t>(work_fn)]
		(auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			asio::post(exec, [
				posted_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				auto result = posted_func();
				asio::dispatch(work.get_executor(), [
					dispatch_result = std::move(result), completion = std::move(handler)
				]() mutable {
					std::move(completion)(std::move(dispatch_result));
				});
			});
		},
		yc);
	}
}

template <concepts::exec YCExec>
auto co_dispatch
(concepts::sched auto &&executor_arg, basic_yield_context<YCExec> yc, concepts::callable auto &&work_fn)
{
	using yield_context = basic_yield_context<YCExec>;
	using function_t = std::decay_t<decltype(work_fn)>;
	using executor_t = std::decay_t<decltype(executor_arg)>;
	using return_t = decltype(work_fn());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>([
			exec = get_executor_helper(std::forward<executor_t>(executor_arg)),
			func = std::forward<function_t>(work_fn)
		](auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			asio::dispatch(exec, [
				dispatched_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				dispatched_func();
				asio::dispatch(work.get_executor(), [completion = std::move(handler)]() mutable {
					std::move(completion)();
				});
			});
		},
		yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_t)>
		([exec = get_executor_helper(executor_arg), func = std::forward<function_t>(work_fn)]
		(auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			asio::dispatch(exec, [
				dispatched_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				auto result = dispatched_func();
				asio::dispatch(work.get_executor(), [
					dispatch_result = std::move(result), completion = std::move(handler)
				]() mutable {
					std::move(completion)(std::move(dispatch_result));
				});
			});
		},
		yc);
	}
}

template <concepts::exec YCExec>
auto co_thread(basic_yield_context<YCExec> yc, concepts::callable auto &&work_fn)
{
	using yield_context = basic_yield_context<YCExec>;
	using function_t = std::decay_t<decltype(work_fn)>;
	using return_t = decltype(work_fn());

	if constexpr( std::is_same_v<return_t, void> )
	{
		return asio::async_initiate<yield_context, void()>
		([func = std::forward<function_t>(work_fn)](auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			std::thread([
				thread_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				thread_func();
				asio::dispatch(work.get_executor(), [completion = std::move(handler)]() mutable {
					std::move(completion)();
				});
			})
			.detach();
		},
		yc);
	}
	else
	{
		return asio::async_initiate<yield_context, void(return_t)>
		([func = std::forward<function_t>(work_fn)](auto completion_handler)
		{
			auto work_guard = asio::make_work_guard(completion_handler);
			std::thread([
				thread_func = std::move(func), handler = std::move(completion_handler),
				work = std::move(work_guard)
			]() mutable
			{
				auto result = thread_func();
				asio::dispatch(work.get_executor(), [
					dispatch_result = std::move(result), completion = std::move(handler)
				]() mutable {
					std::move(completion)(std::move(dispatch_result));
				});
			})
			.detach();
		},
		yc);
	}
}

namespace detail
{

template<concepts::exec YCExec, typename Exec>
error_code sleep_x(const auto &stdtime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	error_code error;
	asio::steady_timer timer(std::forward<Exec>(exec), stdtime);
	timer.async_wait(yc[error]);
	return error;
}

} //namespace detail

template<typename Rep, typename Period, concepts::exec YCExec, concepts::sched Exec>
error_code sleep_for
(const std::chrono::duration<Rep,Period> &rtime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	return detail::sleep_x(rtime, yc, std::forward<Exec>(exec));
}

template<typename Clock, typename Duration, concepts::exec YCExec, concepts::sched Exec>
error_code sleep_until
(const std::chrono::time_point<Clock,Duration> &atime, basic_yield_context<YCExec> yc, Exec &&exec)
{
	return detail::sleep_x(atime, yc, std::forward<Exec>(exec));
}

template <typename T, concepts::exec YCExec>
T wait(basic_yield_context<YCExec> yc, const std::future<T> &future)
{
	return co_thread(yc, [&future] {
		return remove_const(future).get();
	});
}

template <concepts::exec YCExec>
void wait(basic_yield_context<YCExec> yc, const asio::thread_pool &pool)
{
	co_thread(yc, [&pool] {
		return remove_const(pool).wait();
	});
}

template <concepts::exec YCExec>
void wait(basic_yield_context<YCExec> yc, const std::thread &thread)
{
	co_thread(yc, [&thread] {
		return remove_const(thread).join();
	});
}

template <concepts::exec YCExec, concepts::sched Exec>
asio::any_io_executor goto_exec(basic_yield_context<YCExec> yc, Exec &&executor_arg)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>
	([exec = get_executor_helper(std::forward<Exec>(executor_arg))](auto completion_handler)
	{
		auto work_guard = asio::make_work_guard(completion_handler);
		asio::post(exec, [
			handler = std::move(completion_handler), work = std::move(work_guard)
		]() mutable {
			std::move(handler)(work.get_executor());
		});
	},
	yc);
}

template <concepts::exec YCExec>
asio::any_io_executor goto_thread(basic_yield_context<YCExec> yc)
{
	return asio::async_initiate<basic_yield_context<YCExec>, void()>([](auto completion_handler)
	{
		auto work_guard = asio::make_work_guard(completion_handler);
		std::thread([
			handler = std::move(completion_handler), work = std::move(work_guard)
		]() mutable {
			std::move(handler)(work.get_executor());
		}).detach();
	},
	yc);
}

template <concepts::exec Exec>
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

} //namespace libgs::coro


#endif //LIBGS_CORO_DETAIL_UTILS_H
