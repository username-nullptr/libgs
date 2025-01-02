
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
LIBGS_CORE_TAPI auto make_dispatch_lambda(Func &&func, bool &finished)
{
	using return_t = std::invoke_result_t<Func>;
	auto counter = std::make_shared<size_t>(0);

	if constexpr( is_awaitable_v<return_t> )
	{
		using co_return_t = typename return_t::value_type;
		if constexpr( std::is_void_v<co_return_t> )
		{
			auto lambda = [counter, &finished, func = std::forward<Func>(func)]()
			mutable -> awaitable<std::shared_ptr<size_t>>
			{
				co_await func();
				finished = true;
				co_return counter;
			};
			return std::make_pair(std::move(lambda), counter);
		}
		else
		{
			auto lambda = [counter, &finished, func = std::forward<Func>(func)]()
			mutable -> awaitable<std::pair<co_return_t,std::shared_ptr<size_t>>>
			{
				auto res = co_await func();
				finished = true;
				co_return std::make_pair(std::move(res), counter);
			};
			return std::make_pair(std::move(lambda), counter);
		}
	}
	else
	{
		auto lambda = [counter, &finished, func = std::forward<Func>(func)]() mutable
		{
			if constexpr( std::is_void_v<return_t> )
			{
				func();
				finished = true;
				return counter;
			}
			else
			{
				auto res = func();
				finished = true;
				return std::make_pair(std::move(res), counter);
			}
		};
		return std::make_pair(std::move(lambda), counter);
	}
}

LIBGS_CORE_TAPI size_t dispatch_poll(auto &exec, bool &finished)
{
	size_t counter = 0;
	do { counter += exec.poll(); }
	while( not finished );
	return counter;
}

} //namespace detail

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
decltype(auto) dispatch(concepts::schedulable auto &&exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return dispatch(exec, [a = std::forward<Work>(work)]() mutable -> work_t {
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
				std::promise<return_t> promise;
				auto future = promise.get_future();

				asio::dispatch(exec,
				[promise = std::move(promise), func = std::forward<Work>(work)]() mutable {
					detail::promise_set_value(promise, std::forward<Work>(func));
				});
				return return_nodiscard(std::move(future));
			}
			else if constexpr( is_async_opt_token_v<ntoken_t> )
			{
				return asio::co_spawn(exec,
				[func = std::forward<Work>(work)]() mutable -> awaitable<return_t> {
					co_return func();
				}, std::forward<Token>(token));
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
decltype(auto) post(concepts::schedulable auto &&exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return post(exec, [a = std::forward<Work>(work)]() mutable -> work_t {
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
				std::promise<return_t> promise;
				auto future = promise.get_future();

				asio::post(exec,
				[promise = std::move(promise), func = std::forward<Work>(work)]() mutable {
					detail::promise_set_value(promise, std::forward<Work>(func));
				});
				return return_nodiscard(std::move(future));
			}
			else if constexpr( is_async_opt_token_v<ntoken_t> )
			{
				return asio::co_spawn(exec,
				[func = std::forward<Work>(work)]() mutable -> awaitable<return_t> {
					co_return func();
				}, std::forward<Token>(token));
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

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
auto local_dispatch(concepts::execution_context auto &exec, Work &&work, Token &&token)
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch(exec, [a = std::forward<Work>(work)]() mutable -> work_t {
			co_return co_await std::move(a);
		}, std::forward<Token>(token));
	}
	else
	{
		using return_t = std::invoke_result_t<Work>;
		using token_t = std::remove_cvref_t<decltype(token)>;
		using ntoken_t = token_unbound_t<token_t>;

		auto finished = std::make_shared<bool>(false);
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
			return return_nodiscard(std::move(future));
		}
		else if constexpr( is_async_opt_token_v<ntoken_t> )
		{
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto a = dispatch(exec, std::move(lambda), token);

			std::thread([&exec, finished, counter]() mutable {
				*counter = detail::dispatch_poll(exec, *finished);
			}).detach();
			return return_nodiscard(std::move(a));
		}
		else if constexpr( is_awaitable_v<return_t> )
		{
			using co_return_t = typename return_t::value_type;
			auto counter = std::make_shared<size_t>(0);

			if constexpr( std::is_void_v<co_return_t> )
			{
				asio::co_spawn(exec, [&finished, func = std::forward<Work>(work)]() mutable -> awaitable<void>
				{
					co_await func();
					*finished = true;
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(exec, *finished);
				return counter;
			}
			else
			{
				auto pair = std::make_pair(co_return_t(), counter);
				asio::co_spawn(exec, [&pair, &finished, func = std::forward<Work>(work)]() mutable -> awaitable<void>
				{
					auto res = co_await func();
					*finished = true;
					pair.first = std::move(res);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(exec, *finished);
				return pair;
			}
		}
		else
			return std::make_pair(work(), std::make_shared<size_t>(1));
	}
}

template <typename Work>
auto local_dispatch(concepts::execution_context auto &exec, Work &&work)
	requires concepts::dispatch_token<detached_t, Work>
{
	using work_t = std::remove_cvref_t<Work>;
	if constexpr( is_awaitable_v<work_t> )
	{
		return local_dispatch(exec, [a = std::forward<Work>(work)]() mutable -> work_t {
			co_return co_await std::move(a);
		});
	}
	else
	{
		auto finished = std::make_shared<bool>(false);

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
		using token_t = std::remove_cvref_t<decltype(token)>;

		auto finished = std::make_shared<bool>(false);
		if constexpr( is_detached_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			dispatch(*ioc, std::move(lambda), token);

			std::thread([ioc = std::move(ioc), finished]() mutable {
				detail::dispatch_poll(*ioc, *finished);
			}).detach();
		}
		else if constexpr( is_use_future_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto future = dispatch(*ioc, std::move(lambda), token);

			std::thread([ioc = std::move(ioc), finished, counter]() mutable {
				*counter = detail::dispatch_poll(*ioc, *finished);
			}).detach();
			return return_nodiscard(std::move(future));
		}
		else if constexpr( is_async_opt_token_v<token_t> )
		{
			auto ioc = std::make_shared<asio::io_context>();
			auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
			auto a = dispatch(*ioc, std::move(lambda), token);

			std::thread([ioc = std::move(ioc), finished, counter]() mutable {
				*counter = detail::dispatch_poll(*ioc, *finished);
			}).detach();
			return return_nodiscard(std::move(a));
		}
		else if constexpr( is_awaitable_v<return_t> )
		{
			using co_return_t = typename return_t::value_type;
			auto counter = std::make_shared<size_t>(0);
			asio::io_context ioc;

			if constexpr( std::is_void_v<co_return_t> )
			{
				asio::co_spawn(ioc, [&finished, func = std::forward<Work>(work)]() mutable -> awaitable<void>
				{
					co_await func();
					*finished = true;
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(ioc, *finished);
				return counter;
			}
			else
			{
				auto pair = std::make_pair(co_return_t(), counter);
				asio::co_spawn(ioc, [&pair, &finished, func = std::forward<Work>(work)]() mutable -> awaitable<void>
				{
					auto res = co_await func();
					*finished = true;
					pair.first = std::move(res);
					co_return ;
				},
				detached);
				*counter = detail::dispatch_poll(ioc, *finished);
				return pair;
			}
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
		auto finished = std::make_shared<bool>(false);

		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<Work>(work), *finished);
		dispatch(*ioc, std::move(lambda), detached);

		return std::thread([ioc = std::move(ioc), finished]() mutable {
			detail::dispatch_poll(*ioc, *finished);
		});
	}
}

namespace concepts::detail
{

template <typename WakeUp, typename...Args>
concept async_wake_up = requires(WakeUp wake_up, Args&&...args) {
	wake_up(std::move(args)...);
};

} //namespace concepts::detail

namespace detail
{

template <typename Exec, typename WakeUp, typename Handler>
LIBGS_CORE_TAPI void async_xx(const Exec &exec, WakeUp &&wake_up, Handler &&handler)
{
	using handler_t = std::remove_cvref_t<decltype(handler)>;
	using exec_t = std::remove_cvref_t<decltype(exec)>;

	if constexpr( concepts::detail::async_wake_up<WakeUp, handler_t, exec_t> )
		wake_up(std::forward<Handler>(handler), exec);
	else if constexpr( concepts::detail::async_wake_up<WakeUp, handler_t> )
		wake_up(std::forward<Handler>(handler));
	else
		static_assert(false, "Invalid function signature for async");
}

template <typename WakeUp, typename Handler>
LIBGS_CORE_TAPI void async_x(WakeUp &&wake_up, Handler &&handler)
{
	auto work = asio::make_work_guard(handler);
	auto exec = work.get_executor();
	asio::dispatch(exec, [
		exec, wake_up = std::forward<WakeUp>(wake_up),
		handler = std::forward<Handler>(handler)
	]() mutable {
		async_xx(exec, std::move(wake_up), std::move(handler));
	});
}

template <typename Exec, typename WakeUp, typename Handler>
LIBGS_CORE_TAPI void async_x(const Exec &exec, WakeUp &&wake_up, Handler &&handler)
{
	auto work = asio::make_work_guard(handler);
	asio::dispatch(exec, [
		exec = work.get_executor(), wake_up = std::forward<WakeUp>(wake_up),
		handler = std::forward<Handler>(handler)
	]() mutable {
		async_xx(exec, std::move(wake_up), std::move(handler));
	});
}

} //namespace detail

template <concepts::execution Exec, typename...Args>
template <concepts::async_opt_token<Args...> Token>
auto basic_async_work<Exec,Args...>::handle
(concepts::schedulable auto &&exec, concepts::async_wake_up<handler_t&&> auto &&wake_up, Token &&token)
{
	using token_t = std::remove_cvref_t<decltype(token)>;
	using func_t = decltype(wake_up);
	auto ntoken = async_opt_token_helper(token);

	if constexpr( sizeof...(Args) == 1 and std::is_void_v<std::tuple_element_t<0,std::tuple<Args...>>> )
	{
		return asio::async_initiate<token_t, void()> (
		[exec = get_executor_helper(exec), wake_up = std::forward<func_t>(wake_up)](auto handler) mutable {
			detail::async_x(exec, std::move(wake_up), std::move(handler));
		}, ntoken);
	}
	else
	{
		return asio::async_initiate<token_t, void(Args...)> (
		[exec = get_executor_helper(exec), wake_up = std::forward<func_t>(wake_up)](auto handler) mutable {
			detail::async_x(exec, std::move(wake_up), std::move(handler));
		}, ntoken);
	}
}

template <concepts::execution Exec, typename...Args>
template <concepts::async_opt_token<Args...> Token>
auto basic_async_work<Exec,Args...>::handle(concepts::async_wake_up<handler_t&&> auto &&wake_up, Token &&token)
{
	using token_t = std::remove_cvref_t<decltype(token)>;
	using func_t = decltype(wake_up);
	auto ntoken = async_opt_token_helper(token);

	if constexpr( sizeof...(Args) == 1 and std::is_void_v<std::tuple_element_t<0,std::tuple<Args...>>> )
	{
		return asio::async_initiate<token_t, void()> (
		[wake_up = std::forward<func_t>(wake_up)](auto handler) mutable {
			detail::async_x(std::move(wake_up), std::move(handler));
		}, ntoken);
	}
	else
	{
		return asio::async_initiate<token_t, void(Args...)> (
		[wake_up = std::forward<func_t>(wake_up)](auto handler) mutable {
			detail::async_x(std::move(wake_up), std::move(handler));
		}, ntoken);
	}
}

void delete_later(const concepts::execution auto &exec, auto *obj)
{
	asio::post(exec, [obj]{ delete obj; });
}

void delete_later(concepts::execution_context auto &exec, auto *obj)
{
	asio::post(exec, [obj]{ delete obj; });
}

void delete_later(auto *obj)
{
	delete_later(io_context(), obj);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_EXECUTION_H
