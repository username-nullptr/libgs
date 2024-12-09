
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

template <typename Exec, typename Func>
LIBGS_CORE_TAPI [[nodiscard]] auto dispatch_future(Exec &&exec, Func &&func)
{
	using return_t = std::invoke_result_t<Func>;
	if constexpr( is_awaitable_v<return_t> )
		return asio::co_spawn(std::forward<Exec>(exec), std::forward<Func>(func), use_future);
	else
	{
		std::promise<return_t> promise;
		auto future = promise.get_future();

		asio::dispatch(std::forward<Exec>(exec),
		[promise = std::move(promise), func = std::forward<Func>(func)]() mutable {
			promise_set_value(promise, std::forward<Func>(func));
		});
		return nodiscard_return_helper(std::move(future));
	}
}

template <typename Exec, typename Func>
LIBGS_CORE_TAPI [[nodiscard]] auto post_future(Exec &&exec, Func &&func)
{
	using return_t = std::invoke_result_t<Func>;
	if constexpr( is_awaitable_v<return_t> )
		return asio::co_spawn(std::forward<Exec>(exec), std::forward<Func>(func), use_future);
	else
	{
		std::promise<return_t> promise;
		auto future = promise.get_future();

		asio::post(std::forward<Exec>(exec),
		[promise = std::move(promise), func = std::forward<Func>(func)]() mutable {
			promise_set_value(promise, std::forward<Func>(func));
		});
		return nodiscard_return_helper(std::move(future));
	}
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

template <concepts::dispatch_token Token>
auto dispatch(const concepts::execution auto &exec, concepts::callable auto &&func, Token &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	using return_t = std::invoke_result_t<func_t>;
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_detached_v<token_t> )
	{
		if constexpr( is_awaitable_v<return_t> )
			asio::co_spawn(exec, std::forward<func_t>(func), token);
		else
			asio::dispatch(exec, std::forward<func_t>(func));
	}
	else if constexpr( is_use_future_v<token_t> )
		return detail::dispatch_future(exec, std::forward<func_t>(func));

	else if constexpr( is_use_awaitable_v<token_t> )
	{
		if constexpr( is_awaitable_v<return_t> )
			return asio::co_spawn(exec, std::forward<func_t>(func), token);
		else
		{
			return asio::co_spawn(exec,
			[func = std::forward<func_t>(func)]() mutable -> awaitable<return_t> {
				co_return func();
			},
			token);
		}
	}
	else if constexpr( is_awaitable_v<return_t> )
		return detail::dispatch_future(exec, std::forward<func_t>(func)).get();
	else
		return func();
}

template <concepts::dispatch_token Token>
auto dispatch(concepts::execution_context auto &exec, concepts::callable auto &&func, Token &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	return dispatch(exec.get_executor(), std::forward<func_t>(func), std::forward<Token>(token));
}

template <concepts::dispatch_token Token>
auto dispatch(concepts::callable auto &&func, Token &&token)
{
	using func_t = decltype(func);
	return dispatch(execution::context(), std::forward<func_t>(func), std::forward<Token>(token));
}

template <concepts::dispatch_token Token>
auto post(const concepts::execution auto &exec, concepts::callable auto &&func, Token &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	using return_t = std::invoke_result_t<func_t>;
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_detached_v<token_t> )
	{
		if constexpr( is_awaitable_v<return_t> )
			return asio::co_spawn(exec, std::forward<func_t>(func), token);
		else
			return asio::post(exec, std::forward<func_t>(func));
	}
	else if constexpr( is_use_future_v<token_t> )
		return detail::post_future(exec, std::forward<func_t>(func));

	else if constexpr( is_use_awaitable_v<token_t> )
	{
		if constexpr( is_awaitable_v<return_t> )
			return asio::co_spawn(exec, std::forward<func_t>(func), token);
		else
		{
			return asio::co_spawn(exec,
			[func = std::forward<func_t>(func)]() -> awaitable<return_t> {
				co_return func();
			},
			token);
		}
	}
	else if constexpr( is_awaitable_v<return_t> )
		return detail::post_future(exec, std::forward<func_t>(func)).get();
	else
		return func();
}

template <concepts::dispatch_token Token>
auto post(concepts::execution_context auto &exec, concepts::callable auto &&func, Token &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	return post(exec.get_executor(), std::forward<func_t>(func), std::forward<Token>(token));
}

template <concepts::dispatch_token Token>
auto post(concepts::callable auto &&func, Token &&token)
{
	using func_t = decltype(func);
	return post(execution::context(), std::forward<func_t>(func), std::forward<Token>(token));
}

auto local_dispatch(concepts::execution_context auto &exec, concepts::callable auto &&func,
                    concepts::dispatch_token auto &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	using return_t = std::invoke_result_t<func_t>;
	using token_t = std::remove_cvref_t<decltype(token)>;

	auto finished = std::make_shared<bool>(false);
	if constexpr( is_detached_v<token_t> )
	{
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		dispatch(exec, std::move(lambda), token);

		std::thread([&exec, finished]() mutable {
			detail::dispatch_poll(exec, *finished);
		}).detach();
	}
	else if constexpr( is_use_future_v<token_t> )
	{
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		auto future = dispatch(exec, std::move(lambda), token);

		std::thread([&exec, finished, counter]() mutable {
			*counter = detail::dispatch_poll(exec, *finished);
		}).detach();
		return nodiscard_return_helper(std::move(future));
	}
	else if constexpr( is_use_awaitable_v<token_t> )
	{
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		auto a = dispatch(exec, std::move(lambda), token);

		std::thread([&exec, finished, counter]() mutable {
			*counter = detail::dispatch_poll(exec, *finished);
		}).detach();
		return nodiscard_return_helper(std::move(a));
	}
	else if constexpr( is_awaitable_v<return_t> )
	{
		using co_return_t = typename return_t::value_type;
		auto counter = std::make_shared<size_t>(0);

		if constexpr( std::is_void_v<co_return_t> )
		{
			asio::co_spawn(exec, [&finished, func = std::forward<func_t>(func)]() mutable -> awaitable<void>
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
			asio::co_spawn(exec, [&pair, &finished, func = std::forward<func_t>(func)]() mutable -> awaitable<void>
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
		return std::make_pair(func(), std::make_shared<size_t>(1));
}

auto local_dispatch(concepts::execution_context auto &exec, concepts::callable auto &&func)
{
	auto finished = std::make_shared<bool>(false);
	using func_t = std::remove_cvref_t<decltype(func)>;

	auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
	dispatch(exec, std::move(lambda), detached);

	return std::thread([&exec, finished]() mutable {
		detail::dispatch_poll(exec, *finished);
	});
}

auto local_dispatch(concepts::callable auto &&func, concepts::dispatch_token auto &&token)
{
	using func_t = std::remove_cvref_t<decltype(func)>;
	using return_t = std::invoke_result_t<func_t>;
	using token_t = std::remove_cvref_t<decltype(token)>;

	auto finished = std::make_shared<bool>(false);
	if constexpr( is_detached_v<token_t> )
	{
		auto ioc = std::make_shared<asio::io_context>();
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		dispatch(*ioc, std::move(lambda), token);

		std::thread([ioc = std::move(ioc), finished]() mutable {
			detail::dispatch_poll(*ioc, *finished);
		}).detach();
	}
	else if constexpr( is_use_future_v<token_t> )
	{
		auto ioc = std::make_shared<asio::io_context>();
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		auto future = dispatch(*ioc, std::move(lambda), token);

		std::thread([ioc = std::move(ioc), finished, counter]() mutable {
			*counter = detail::dispatch_poll(*ioc, *finished);
		}).detach();
		return nodiscard_return_helper(std::move(future));
	}
	else if constexpr( is_use_awaitable_v<token_t> )
	{
		auto ioc = std::make_shared<asio::io_context>();
		auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
		auto a = dispatch(*ioc, std::move(lambda), token);

		std::thread([ioc = std::move(ioc), finished, counter]() mutable {
			*counter = detail::dispatch_poll(*ioc, *finished);
		}).detach();
		return nodiscard_return_helper(std::move(a));
	}
	else if constexpr( is_awaitable_v<return_t> )
	{
		using co_return_t = typename return_t::value_type;
		auto counter = std::make_shared<size_t>(0);
		asio::io_context ioc;

		if constexpr( std::is_void_v<co_return_t> )
		{
			asio::co_spawn(ioc, [&finished, func = std::forward<func_t>(func)]() mutable -> awaitable<void>
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
			asio::co_spawn(ioc, [&pair, &finished, func = std::forward<func_t>(func)]() mutable -> awaitable<void>
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
		return std::make_pair(func(), std::make_shared<size_t>(1));

}

auto local_dispatch(concepts::callable auto &&func)
{
	auto ioc = std::make_shared<asio::io_context>();
	auto finished = std::make_shared<bool>(false);
	using func_t = std::remove_cvref_t<decltype(func)>;

	auto [lambda, counter] = detail::make_dispatch_lambda(std::forward<func_t>(func), *finished);
	dispatch(*ioc, std::move(lambda), detached);

	return std::thread([ioc = std::move(ioc), finished]() mutable {
		detail::dispatch_poll(*ioc, *finished);
	});
}

template <typename...Args>
auto asnyc(concepts::function auto &&wake_up, concepts::async_opt_token<Args...> auto &&token)
{
	using token_t = std::remove_cvref_t<decltype(token)>;
	using func_t = decltype(wake_up);

	constexpr bool is_void = sizeof...(Args) == 0 or
		(sizeof...(Args) == 1 and std::is_void_v<std::tuple_element_t<0,std::tuple<Args...>>>);

	if constexpr( is_void )
	{
		return asio::async_initiate<token_t, void()>([wake_up = std::forward<func_t>(wake_up)](auto handler) mutable
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(work.get_executor(),
			[wake_up = std::move(wake_up), handler = std::move(handler)]() mutable {
				wake_up(std::move(handler));
			});
		},
		co_opt_token_helper(token));
	}
	else
	{
		return asio::async_initiate<token_t, void(Args...)>([wake_up = std::forward<func_t>(wake_up)](auto handler) mutable
		{
			auto work = asio::make_work_guard(handler);
			asio::dispatch(work.get_executor(),
			[wake_up = std::move(wake_up), handler = std::move(handler)]() mutable {
				wake_up(std::move(handler));
			});
		},
		co_opt_token_helper(token));
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
	delete_later(execution::context(), obj);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_EXECUTION_H
