/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_UTILS_DETAIL_ASYNC_EXPECTED_H
#define LIBGS_HTTP_UTILS_DETAIL_ASYNC_EXPECTED_H

#include <libgs/coro/utils.h>

namespace libgs::http::detail
{

template <typename Buffer, typename Source>
[[nodiscard]] Buffer copy_buffer_data(Source &&source)
	requires (not is_array_buffer_v<Buffer>)
{
	using source_t = std::remove_cvref_t<Source>;
	if constexpr( std::same_as<Buffer,source_t> )
	{
		return std::forward<Source>(source);
	}
	else
	{
		using value_t = typename Buffer::value_type;
		const auto byte_size = source.size() * sizeof(typename source_t::value_type);
		const auto value_size = byte_size / sizeof(value_t) +
			static_cast<size_t>(byte_size % sizeof(value_t) != 0);

		Buffer result {};
		result.resize(value_size);
		if( byte_size > 0 )
		{
			std::memcpy(result.data(), source.data(), byte_size);
		}
		return result;
	}
}

template <typename Value>
[[nodiscard]] Value expected_value_or_throw(sys_expected<Value> expected)
{
	if( not expected )
		throw std::system_error(expected.error());
	return std::move(*expected);
}

template <typename Value>
[[nodiscard]] Value expected_value_or_error(sys_expected<Value> expected, error_code &error)
	noexcept(std::is_nothrow_move_constructible_v<Value>)
{
	if( not expected )
	{
		error = expected.error();
		return {};
	}
	error.clear();
	return std::move(*expected);
}

template <typename>
struct is_async_argument_reference : std::false_type {};

template <typename T>
struct is_async_argument_reference<std::reference_wrapper<T>> : std::true_type {};

template <typename T>
[[nodiscard]] auto capture_async_argument(T &&value)
{
	if constexpr( std::is_lvalue_reference_v<T> )
	{
		return std::ref(value);
	}
	else
	{
		return std::remove_cvref_t<T>(std::forward<T>(value));
	}
}

template <typename T>
[[nodiscard]] decltype(auto) unwrap_async_argument(T &value) noexcept
{
	if constexpr( is_async_argument_reference<std::remove_cvref_t<T>>::value )
	{
		return value.get();
	}
	else
	{
		return (value);
	}
}

template <typename Value, typename Factory>
[[nodiscard]] awaitable<sys_expected<Value>> co_expected_with_timeout
(Factory factory, std::chrono::nanoseconds timeout)
{
	if( timeout <= std::chrono::nanoseconds::zero() )
		co_return co_await factory();

	auto exec = co_await asio::this_coro::executor;
	using namespace asio::experimental::awaitable_operators;

	auto result = co_await (
		factory() or coro::sleep_for(exec, timeout)
	);
	if( result.index() == 0 )
		co_return std::move(std::get<0>(result));

	const auto &timer_error = std::get<1>(result);
	if( timer_error )
		co_return sys_unexpected(timer_error);
	co_return sys_unexpected(make_error_code(errc::timed_out));
}

template <typename Value, typename Factory, typename Handler>
[[nodiscard]] awaitable<void> co_complete_expected
(Factory factory, std::chrono::nanoseconds timeout, std::shared_ptr<Handler> handler)
{
	error_code error {};
	Value value {};
	try
	{
		auto result = co_await co_expected_with_timeout<Value>(
			std::move(factory), timeout
		);
		if( result )
			value = std::move(*result);
		else
			error = result.error();
	}
	catch(const std::system_error &ex)
	{
		error = ex.code();
	}
	catch(const std::bad_alloc&)
	{
		error = make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		error = make_error_code(std::errc::io_error);
	}
	std::move(*handler)(error, std::move(value));
	co_return ;
}

template <typename Value, core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected(const Exec &exec, Factory factory, Token &&token)
{
	auto timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
		get_associated_redirect_time(token)
	);
	decltype(auto) no_time_token = unbound_redirect_time(
		std::forward<Token>(token)
	);
	return async_work<error_code,Value>::handle(exec,
	[exec, factory = std::move(factory), timeout](auto handler) mutable
	{
		using handler_t = std::remove_cvref_t<decltype(handler)>;
		auto handler_ptr = std::make_shared<handler_t>(std::move(handler));
		auto slot = asio::get_associated_cancellation_slot(*handler_ptr);

		asio::co_spawn(exec,
			co_complete_expected<Value>(
				std::move(factory), timeout, std::move(handler_ptr)
			),
			asio::bind_cancellation_slot(slot, detached)
		);
	},
	std::forward<decltype(no_time_token)>(no_time_token));
}

template <typename Factory>
[[nodiscard]] awaitable<sys_expected<std::monostate>> co_void_expected_value
(Factory factory)
{
	auto result = co_await factory();
	if( not result )
		co_return sys_unexpected(result.error());
	co_return std::monostate {};
}

template <typename Factory, typename Handler>
[[nodiscard]] awaitable<void> co_complete_expected_void
(Factory factory, std::chrono::nanoseconds timeout, std::shared_ptr<Handler> handler)
{
	error_code error {};
	try
	{
		auto result = co_await co_expected_with_timeout<std::monostate>(
			[factory = std::move(factory)]() mutable
			{
				return co_void_expected_value(std::move(factory));
			},
			timeout
		);
		if( not result )
			error = result.error();
	}
	catch(const std::system_error &ex)
	{
		error = ex.code();
	}
	catch(const std::bad_alloc&)
	{
		error = make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		error = make_error_code(std::errc::io_error);
	}
	std::move(*handler)(error);
	co_return ;
}

template <core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_void
(const Exec &exec, Factory factory, Token &&token)
{
	auto timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(
		get_associated_redirect_time(token)
	);
	decltype(auto) no_time_token = unbound_redirect_time(
		std::forward<Token>(token)
	);
	return async_work<error_code>::handle(exec,
	[exec, factory = std::move(factory), timeout](auto handler) mutable
	{
		using handler_t = std::remove_cvref_t<decltype(handler)>;
		auto handler_ptr = std::make_shared<handler_t>(std::move(handler));
		auto slot = asio::get_associated_cancellation_slot(*handler_ptr);

		asio::co_spawn(exec,
			co_complete_expected_void(
				std::move(factory), timeout, std::move(handler_ptr)
			),
			asio::bind_cancellation_slot(slot, detached)
		);
	},
	std::forward<decltype(no_time_token)>(no_time_token));
}

} //namespace libgs::http::detail


#endif //LIBGS_HTTP_UTILS_DETAIL_ASYNC_EXPECTED_H
