// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_DETAIL_EXECUTION_H
#define LIBGS_CORE_DETAIL_EXECUTION_H

namespace libgs { namespace detail
{

template <typename T>
LIBGS_CORE_TAPI void promise_set_value(std::promise<T> &promise, auto &&func)
{
	if constexpr( std::is_void_v<T> )
	{
		func();
		promise.set_value();
	}
	else
		promise.set_value(func());
}

template <typename Func>
LIBGS_CORE_TAPI auto make_dispatch_lambda(Func &&work, std::atomic_bool &finished)
{
	using return_t = std::invoke_result_t<Func>;
	auto counter = std::make_shared<size_t>(0);

	if constexpr( is_awaitable_v<return_t> )
	{
		using co_return_t = return_t::value_type;
		if constexpr( std::is_void_v<co_return_t> )
		{
			auto lambda = [counter, &finished, func = std::forward<Func>(work)]()
			mutable noexcept -> awaitable<std::shared_ptr<size_t>>
			{
				co_await func();
				finished.store(true, std::memory_order_release);
				co_return counter;
			};
			return std::make_pair(std::move(lambda), counter);
		}
		else
		{
			auto lambda = [counter, &finished, func = std::forward<Func>(work)]()
			mutable noexcept -> awaitable<std::pair<co_return_t,std::shared_ptr<size_t>>>
			{
				auto res = co_await func();
				finished.store(true, std::memory_order_release);
				co_return std::make_pair(std::move(res), counter);
			};
			return std::make_pair(std::move(lambda), counter);
		}
	}
	else
	{
		auto lambda = [counter, &finished, func = std::forward<Func>(work)]() mutable noexcept
		{
			if constexpr( std::is_void_v<return_t> )
			{
				func();
				finished.store(true, std::memory_order_release);
				return counter;
			}
			else
			{
				auto res = func();
				finished.store(true, std::memory_order_release);
				return std::make_pair(std::move(res), counter);
			}
		};
		return std::make_pair(std::move(lambda), counter);
	}
}

LIBGS_CORE_TAPI size_t dispatch_poll(auto &exec, std::atomic_bool &finished)
{
	size_t counter = 0;
	do { counter += exec.poll(); }
	while( not finished.load(std::memory_order_acquire) );
	return counter;
}

} //namespace detail

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
decltype(auto) dispatch(concepts::sched auto &&exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return dispatch(exec, [a = std::forward<Work>(work)]() mutable noexcept -> work_t {
			co_return co_await std::move(a);
		}, std::forward<Token>(token));
	}
	else
	{
		using return_t = std::invoke_result_t<Work>;
		using token_t = std::remove_cvref_t<Token>;

		if constexpr( is_awaitable_v<return_t> )
		{
			if constexpr( is_sync_opt_token_v<Token> )
				return dispatch(exec, std::forward<Work>(work), use_future).get();
			else
				return asio::co_spawn(exec, std::forward<Work>(work), std::forward<Token>(token));
		}
		else
		{
			using ntoken_t = token_unbound_t<token_t>;
			if constexpr( is_detached_v<ntoken_t> )
				asio::dispatch(exec, std::forward<Work>(work));

			else if constexpr( is_use_future_v<ntoken_t> )
			{
				std::promise<return_t> result_promise;
				auto future = result_promise.get_future();

				asio::dispatch(exec,
				[promise = std::move(result_promise), func = std::forward<Work>(work)]() mutable noexcept {
					detail::promise_set_value(promise, func);
				});
				return future;
			}
			else if constexpr( is_async_opt_token_v<ntoken_t> )
			{
				return asio::co_spawn(exec,
				[func = std::forward<Work>(work)]() mutable noexcept -> awaitable<return_t> {
					co_return func();
				}, std::forward<Token>(token));
			}
			else if constexpr( std::is_lvalue_reference_v<return_t> )
			{
				using nr_return_t = std::remove_reference_t<return_t>;
				std::promise<nr_return_t*> result_promise;
				auto future = result_promise.get_future();

				asio::dispatch(exec,
				[promise = std::move(result_promise), func = std::forward<Work>(work)]() mutable noexcept
				{
					detail::promise_set_value(promise, [reference_func = std::forward<Work>(func)]() mutable noexcept {
						return &reference_func();
					});
				});
				return return_reference(*future.get());
			}
			else
				return dispatch(exec, std::forward<Work>(work), use_future).get();
		}
	}
}

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
decltype(auto) dispatch(Work &&work, Token &&token)
{
	return dispatch(io_context(), std::forward<Work>(work), std::forward<Token>(token));
}

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
decltype(auto) post(concepts::sched auto &&exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return post(exec, [a = std::forward<Work>(work)]() mutable noexcept -> work_t {
			co_return co_await std::move(a);
		}, std::forward<Token>(token));
	}
	else
	{
		using return_t = std::invoke_result_t<Work>;
		using token_t = std::remove_cvref_t<Token>;

		if constexpr( is_awaitable_v<return_t> )
		{
			if constexpr( is_sync_opt_token_v<Token> )
				return post(exec, std::forward<Work>(work), use_future).get();
			else
				return asio::co_spawn(exec, std::forward<Work>(work), std::forward<Token>(token));
		}
		else
		{
			using ntoken_t = token_unbound_t<token_t>;
			if constexpr( is_detached_v<ntoken_t> )
				asio::post(exec, std::forward<Work>(work));

			else if constexpr( is_use_future_v<ntoken_t> )
			{
				std::promise<return_t> result_promise;
				auto future = result_promise.get_future();

				asio::post(exec,
				[promise = std::move(result_promise), func = std::forward<Work>(work)]() mutable noexcept {
					detail::promise_set_value(promise, func);
				});
				return future;
			}
			else if constexpr( is_async_opt_token_v<ntoken_t> )
			{
				return asio::co_spawn(exec,
				[func = std::forward<Work>(work)]() mutable noexcept -> awaitable<return_t> {
					co_return func();
				}, std::forward<Token>(token));
			}
			else if constexpr( std::is_lvalue_reference_v<return_t> )
			{
				using nr_return_t = std::remove_reference_t<return_t>;
				std::promise<nr_return_t*> result_promise;
				auto future = result_promise.get_future();

				asio::post(exec,
				[promise = std::move(result_promise), func = std::forward<Work>(work)]() mutable noexcept
				{
					detail::promise_set_value(promise, [reference_func = std::forward<Work>(func)]() mutable noexcept {
						return &reference_func();
					});
				});
				return return_reference(*future.get());
			}
			else
				return post(exec, std::forward<Work>(work), use_future).get();
		}
	}
}

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
decltype(auto) post(Work &&work, Token &&token)
{
	return post(io_context(), std::forward<Work>(work), std::forward<Token>(token));
}

template <concepts::dispatch_work Work, typename Rep, typename Period>
work_canceller_t post(concepts::sched auto &&exec, const duration<Rep,Period> &rtime, Work &&work)
{
	asio::steady_timer::duration time;
	if constexpr( std::is_signed_v<decltype(rtime.count())> )
	{
		time = rtime.count() <= 0 ? asio::steady_timer::duration(0) :
			std::chrono::duration_cast<asio::steady_timer::duration>(rtime);
	}
	else
	{
		time = rtime.count() == 0 ? asio::steady_timer::duration(0) :
			std::chrono::duration_cast<asio::steady_timer::duration>(rtime);
	}
	auto timer = std::make_shared<asio::steady_timer>(
		std::forward<decltype(exec)>(exec), time
	);
	work_canceller_t cancel = [timer]() mutable {
		timer->cancel();
	};
	timer->async_wait([timer,
		scheduled_exec = get_executor_helper(exec),
		scheduled_work = std::forward<Work>(work)
	](const error_code &error) mutable
	{
		LIBGS_UNUSED(timer);
		if( error != errc::operation_aborted )
			dispatch(std::move(scheduled_exec), std::move(scheduled_work));
	});
	return cancel;
}

template <concepts::dispatch_work Work, typename Rep, typename Period>
work_canceller_t post(const duration<Rep,Period> &rtime, Work &&work)
{
	return post(io_context(), rtime, std::forward<Work>(work));
}

template <concepts::dispatch_work Work, typename Clock, typename Duration>
work_canceller_t post(concepts::sched auto &&exec, const time_point<Clock,Duration> &atime, Work &&work)
{
	auto timer = std::make_shared<asio::basic_waitable_timer<Clock>>(
		std::forward<decltype(exec)>(exec), atime
	);
	work_canceller_t cancel = [timer]() mutable {
		timer->cancel();
	};
	timer->async_wait([timer,
		scheduled_exec = get_executor_helper(exec),
		scheduled_work = std::forward<Work>(work)
	](const error_code &error) mutable
	{
		LIBGS_UNUSED(timer);
		if( error != errc::operation_aborted )
			dispatch(std::move(scheduled_exec), std::move(scheduled_work));
	});
	return cancel;
}

template <concepts::dispatch_work Work, typename Clock, typename Duration>
work_canceller_t post(const time_point<Clock,Duration> &atime, Work &&work)
{
	return post(io_context(), atime, std::forward<Work>(work));
}

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
auto local_dispatch(concepts::exec_context auto &exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch(exec,
		[a = std::forward<Work>(work)]() mutable noexcept -> work_t {
			co_return co_await std::move(a);
		}, std::forward<Token>(token));
	}
	else
	{
		using return_t = std::invoke_result_t<Work>;
		using token_t = std::remove_cvref_t<Token>;
		using ntoken_t = token_unbound_t<token_t>;

		auto finished = std::make_shared<std::atomic_bool>(false);
		if constexpr( is_detached_v<ntoken_t> )
		{
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			dispatch(exec, std::move(lambda), std::forward<Token>(token));

			std::thread([&exec, finished]() mutable {
				detail::dispatch_poll(exec, *finished);
			}).detach();
		}
		else if constexpr( is_use_future_v<ntoken_t> )
		{
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto future = dispatch(exec, std::move(lambda), std::forward<Token>(token));

			std::thread([&exec, finished, counter]() mutable {
				*counter = detail::dispatch_poll(exec, *finished);
			}).detach();
			return std::move(future);
		}
		else if constexpr( is_async_opt_token_v<ntoken_t> )
		{
			// Async tokens may initiate only when their result is awaited.
			auto poll_work = asio::make_work_guard(exec);
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto a = dispatch(exec, std::move(lambda), token);

			std::thread([&exec, finished, counter, poll_work = std::move(poll_work)]() mutable
			{
				LIBGS_UNUSED(poll_work);
				*counter = detail::dispatch_poll(exec, *finished);
			})
			.detach();
			return std::move(a);
		}
		else if constexpr( is_awaitable_v<return_t> )
		{
			using co_return_t = return_t::value_type;
			auto counter = std::make_shared<size_t>(0);

			if constexpr( std::is_void_v<co_return_t> )
			{
				asio::co_spawn(exec, [&finished, func = std::forward<Work>(work)]
				() mutable noexcept -> awaitable<void>
				{
					co_await func();
					finished->store(true, std::memory_order_release);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(exec, *finished);
				return counter;
			}
			else
			{
				auto pair = std::make_pair(co_return_t(), counter);
				asio::co_spawn(exec, [&pair, &finished, func = std::forward<Work>(work)]
				() mutable noexcept -> awaitable<void>
				{
					auto res = co_await func();
					finished->store(true, std::memory_order_release);
					pair.first = std::move(res);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(exec, *finished);
				return pair;
			}
		}
		else if constexpr( std::is_void_v<return_t> )
		{
			work();
			return std::make_shared<size_t>(1);
		}
		else
			return std::make_pair(work(), std::make_shared<size_t>(1));
	}
}

template <typename Work>
auto local_dispatch(concepts::exec_context auto &exec, Work &&work)
	requires concepts::dispatch_token<detached_t, Work>
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch(exec,
		[a = std::forward<Work>(work)]() mutable noexcept -> work_t {
			co_return co_await std::move(a);
		});
	}
	else
	{
		auto finished = std::make_shared<std::atomic_bool>(false);

		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
		dispatch(exec, std::move(lambda), detached);

		return std::thread([&exec, finished]() mutable {
			detail::dispatch_poll(exec, *finished);
		});
	}
}

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
auto local_dispatch(Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch([a = std::forward<Work>(work)]() mutable -> work_t {
			co_return co_await std::move(a);
		}, std::forward<Token>(token));
	}
	else
	{
		using return_t = std::invoke_result_t<Work>;
		using token_t = std::remove_cvref_t<Token>;

		auto finished = std::make_shared<std::atomic_bool>(false);
		if constexpr( is_detached_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			dispatch(*ioc, std::move(lambda), token);

			std::thread([poll_context = std::move(ioc), finished]() mutable {
				detail::dispatch_poll(*poll_context, *finished);
			}).detach();
		}
		else if constexpr( is_use_future_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto future = dispatch(*ioc, std::move(lambda), token);

			std::thread([poll_context = std::move(ioc), finished, counter]() mutable {
				*counter = detail::dispatch_poll(*poll_context, *finished);
			}).detach();
			return std::move(future);
		}
		else if constexpr( is_async_opt_token_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			// Keep the context runnable until the lazy operation is initiated.
			auto poll_work = asio::make_work_guard(*ioc);

			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto a = dispatch(*ioc, std::move(lambda), token);

			std::thread([poll_context = std::move(ioc), finished, counter,
				poll_work = std::move(poll_work)]() mutable
			{
				LIBGS_UNUSED(poll_work);
				*counter = detail::dispatch_poll(*poll_context, *finished);
			}).detach();
			return std::move(a);
		}
		else if constexpr( is_awaitable_v<return_t> )
		{
			using co_return_t = return_t::value_type;
			auto counter = std::make_shared<size_t>(0);
			asio::io_context ioc;

			if constexpr( std::is_void_v<co_return_t> )
			{
				asio::co_spawn(ioc, [&finished, func = std::forward<Work>(work)]
				() mutable noexcept -> awaitable<void>
				{
					co_await func();
					finished->store(true, std::memory_order_release);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(ioc, *finished);
				return counter;
			}
			else
			{
				auto pair = std::make_pair(co_return_t(), counter);
				asio::co_spawn(ioc, [&pair, &finished, func = std::forward<Work>(work)]
				() mutable noexcept -> awaitable<void>
				{
					auto res = co_await func();
					finished->store(true, std::memory_order_release);
					pair.first = std::move(res);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(ioc, *finished);
				return pair;
			}
		}
		else if constexpr( std::is_void_v<return_t> )
		{
			work();
			return std::make_shared<size_t>(1);
		}
		else
			return std::make_pair(work(), std::make_shared<size_t>(1));
	}
}

template <typename Work>
auto local_dispatch(Work &&work)
	requires concepts::dispatch_token<detached_t, Work>
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch([a = std::forward<Work>(work)]() mutable -> work_t {
			co_return co_await std::move(a);
		});
	}
	else
	{
		auto ioc = std::make_shared<asio::io_context>();
		auto finished = std::make_shared<std::atomic_bool>(false);

		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
		dispatch(*ioc, std::move(lambda), detached);

		return std::thread([poll_context = std::move(ioc), finished]() mutable {
			detail::dispatch_poll(*poll_context, *finished);
		});
	}
}

namespace detail
{

template <typename Exec, typename Time, typename Token>
[[nodiscard]] awaitable<error_code> co_sleep_for(Exec exec, Time rtime, Token token)
{
	asio::steady_timer timer(std::move(exec),
		std::chrono::duration_cast<asio::steady_timer::duration>(rtime)
	);
	co_await timer.async_wait(std::move(token));

	using namespace operators;
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_cancellation_slot_binder_v<token_t> )
	{
		auto &target = token.get();
		using target_t = std::remove_cvref_t<decltype(target)>;

		if constexpr( is_redirect_error_v<target_t> )
			co_return target.ec_;
		else
			co_return error_code();
	}
	else if constexpr( is_redirect_error_v<token_t> )
		co_return token.ec_;
	else
		co_return error_code();
}

template <typename Time, typename Token>
[[nodiscard]] awaitable<error_code> co_sleep_for(Time rtime, Token token)
{
	co_return co_await co_sleep_for(co_await asio::this_coro::executor,
		std::move(rtime), std::move(token)
	);
}

template <typename Exec, typename Clock, typename Duration, typename Token>
[[nodiscard]] awaitable<error_code> co_sleep_until(Exec exec, time_point<Clock,Duration> atime, Token token)
{
	asio::basic_waitable_timer<Clock> timer(std::move(exec), atime);
	co_await timer.async_wait(std::move(token));

	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_cancellation_slot_binder_v<token_t> )
	{
		auto &target = token.get();
		using target_t = std::remove_cvref_t<decltype(target)>;
		if constexpr( is_redirect_error_v<target_t> )
			co_return target.ec_;
		else
			co_return error_code();
	}
	else if constexpr( is_redirect_error_v<token_t> )
		co_return token.ec_;
	else
		co_return error_code();
}

template <typename Clock, typename Duration, typename Token>
[[nodiscard]] awaitable<error_code> co_sleep_until(time_point<Clock,Duration> atime, Token token)
{
	co_return co_await co_sleep_until(co_await asio::this_coro::executor,
		std::move(atime), std::move(token)
	);
}

} //namespace detail

template <typename Rep, typename Period, concepts::co_sleep_opt_token Token>
auto sleep_for(concepts::sched auto &&exec, const duration<Rep,Period> &rtime, Token &&token)
{
	using Exec = decltype(exec);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_void_func_v<token_t> )
	{
		auto time = rtime.count() < 0 ? asio::steady_timer::duration(0) :
			std::chrono::duration_cast<asio::steady_timer::duration>(rtime);

		auto timer = std::make_shared<asio::steady_timer>(
			std::forward<Exec>(exec), time
		);
		timer->async_wait(
		[timer, callback = std::forward<Token>(token)](const error_code &error)
		{
			LIBGS_UNUSED(timer);
			callback(error);
		});
	}
	else
	{
		return detail::co_sleep_for(get_executor_helper(std::forward<Exec>(exec)),
			rtime, std::forward<Token>(token)
		);
	}
}

template <typename Rep, typename Period, concepts::sleep_opt_token Token>
auto sleep_for(const duration<Rep,Period> &rtime, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_void_func_v<token_t> )
		sleep_for(get_executor(), rtime, std::forward<Token>(token));

	else if constexpr( is_async_opt_token_v<token_t> )
		return detail::co_sleep_for(rtime, std::forward<Token>(token));
	else
		std::this_thread::sleep_for(rtime);
}

template <typename Clock, typename Duration, concepts::co_sleep_opt_token Token>
auto sleep_until(concepts::sched auto &&exec, const time_point<Clock,Duration> &atime, Token &&token)
{
	using Exec = decltype(exec);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_void_func_v<token_t> )
	{
		auto timer = std::make_shared<asio::basic_waitable_timer<Clock>>(
			get_executor_helper(std::forward<Exec>(exec)), atime
		);
		timer->async_wait(
		[timer, callback = std::forward<Token>(token)](const error_code &error) mutable
		{
			LIBGS_UNUSED(timer);
			callback(error);
		});
	}
	else
	{
		return detail::co_sleep_until(
			get_executor_helper(std::forward<Exec>(exec)),
			atime, std::forward<Token>(token)
		);
	}
}

template <typename Clock, typename Duration, concepts::sleep_opt_token Token>
auto sleep_until(const time_point<Clock,Duration> &atime, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_void_func_v<token_t> )
		sleep_until(get_executor(), atime, std::forward<Token>(token));

	else if constexpr( is_async_opt_token_v<token_t> )
		return detail::co_sleep_until(atime, std::forward<Token>(token));
	else
		std::this_thread::sleep_until(atime);
}

template <concepts::timer_work Work, typename Rep, typename Period>
work_canceller_t start_timer(concepts::sched auto &&exec,
	const duration<Rep,Period> &rtime, Work &&work, bool immediately)
{
	if constexpr( std::is_signed_v<decltype(rtime.count())> )
	{
		if( rtime.count() <= 0 )
		{
			runtime_error::loc_throw (
				"libgs::start_timer: Invalid time duration"
			);
		}
	}
	else
	{
		if( rtime.count() == 0 )
		{
			runtime_error::loc_throw (
				"libgs::start_timer: Invalid time duration"
			);
		}
	}
	auto timer = std::make_shared<asio::steady_timer>(exec);
	auto cancel = std::make_shared<bool>(false);

	work_canceller_t canceller = [timer, cancel]() mutable
	{
		*cancel = true;
		timer->cancel();
	};
	libgs::dispatch(std::forward<decltype(exec)>(exec), [
		timer_ptr = std::move(timer), cancel_flag = std::move(cancel), canceller,
		delay = std::chrono::duration_cast<asio::steady_timer::duration>(rtime),
		func = std::forward<Work>(work), immediately
	]() mutable noexcept -> awaitable<void>
	{
		using namespace operators;
		error_code error;

		auto atime = std::chrono::steady_clock::now();
		auto sleep = [&]() -> awaitable<bool>
		{
			if( *cancel_flag )
				co_return false;

			atime += delay;
			timer_ptr->expires_at(atime);

			co_await timer_ptr->async_wait(use_awaitable | error);
			if( *cancel_flag )
				co_return false;

			co_return not error;
		};
		if( not immediately )
		{
			if( not co_await sleep() )
				co_return ;
		}
		for(;;)
		{
			if constexpr( concepts::callable<Work,work_canceller_t> )
			{
				using return_t = decltype(std::declval<Work>()(canceller));
				if constexpr( is_awaitable_v<return_t> )
					co_await func(canceller);
				else
					func(canceller);
			}
			else
			{
				using return_t = std::invoke_result_t<Work>;
				if constexpr( is_awaitable_v<return_t> )
					co_await func();
				else
					func();
			}
			if( not co_await sleep() )
				break;
		}
		co_return ;
	});
	return canceller;
}

template <concepts::timer_work Work, typename Rep, typename Period>
work_canceller_t start_timer(const duration<Rep,Period> &rtime, Work &&work, bool immediately)
{
	return start_timer(io_context(), rtime, std::forward<Work>(work), immediately);
}

void delete_later(const concepts::exec auto &exec, auto *obj)
{
	asio::post(exec, [obj]{ delete obj; });
}

void delete_later(concepts::exec_context auto &exec, auto *obj)
{
	asio::post(exec, [obj]{ delete obj; });
}

void delete_later(auto *obj)
{
	delete_later(io_context(), obj);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_EXECUTION_H
