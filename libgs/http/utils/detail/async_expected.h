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
		return std::forward<Source>(source);
	else
	{
		using value_t = Buffer::value_type;
		const auto byte_size = source.size() * sizeof(typename source_t::value_type);

		const auto value_size = byte_size / sizeof(value_t) +
			static_cast<size_t>(byte_size % sizeof(value_t) != 0);

		Buffer result {};
		result.resize(value_size);

		if( byte_size > 0 )
			std::memcpy(result.data(), source.data(), byte_size);
		return result;
	}
}

template <typename Value>
[[nodiscard]] Value expected_value_or_throw(sys_expected<Value> expected)
{
	if( not expected )
		system_error::loc_throw(expected.error());
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
		return std::ref(value);
	else
		return std::remove_cvref_t<T>(std::forward<T>(value));
}

template <typename T>
[[nodiscard]] decltype(auto) unwrap_async_argument(T &value) noexcept
{
	if constexpr( is_async_argument_reference<std::remove_cvref_t<T>>::value )
		return value.get();
	else
		return (value);
}

template <typename Value, typename Factory>
[[nodiscard]] awaitable<sys_expected<Value>> co_expected_with_timeout
(Factory factory, std::chrono::nanoseconds timeout)
{
	if( timeout <= std::chrono::nanoseconds::zero() )
		co_return co_await factory();

	auto exec = co_await asio::this_coro::executor;
	using namespace asio::experimental::awaitable_operators;

	// operator|| uses wait_for_one_success(cancellation_type::all). When the
	// timer wins it emits cancellation to factory() through its associated
	// cancellation slot, then waits for the cancelled I/O to finish.
	auto result = co_await (
		factory() or coro::sleep_for(exec, timeout)
	);
	if( result.index() == 0 )
		co_return std::move(std::get<0>(result));

	if( const auto &timer_error = std::get<1>(result) )
		co_return sys_unexpected(timer_error);
	co_return sys_unexpected(make_error_code(errc::timed_out));
}

inline error_code exception_error(const std::exception_ptr &exception) noexcept
{
	if( not exception )
		return {};
	try {
		std::rethrow_exception(exception);
	}
	catch(const std::system_error &ex) {
		return ex.code();
	}
	catch(const std::bad_alloc&) {
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {}
	return make_error_code(std::errc::io_error);
}

template <typename Value, core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_impl
(const Exec &exec, Factory factory_fn,
	std::chrono::nanoseconds timeout, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,Value)>(
	[exec, operation = std::move(factory_fn), timeout]
	(auto completion_handler) mutable
	{
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto completion_exec = asio::get_associated_executor(
			completion_handler, exec
		);
		auto allocator = asio::get_associated_allocator(completion_handler);

		asio::co_spawn(exec,
			co_expected_with_timeout<Value>(std::move(operation), timeout),
			asio::bind_allocator(allocator, asio::bind_executor(completion_exec,
				asio::bind_cancellation_slot(slot,
				[handler = std::move(completion_handler)]
				(const std::exception_ptr &exception, sys_expected<Value> result) mutable
				{
					if( auto error = exception_error(exception) )
					{
						std::move(handler)(error, Value{});
						return ;
					}
					if( not result )
						std::move(handler)(result.error(), Value{});
					else
						std::move(handler)(error_code{}, std::move(*result));
				}))
			)
		);
	},
	completion_token);
}

template <typename Value, core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected(const Exec &exec, Factory factory, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		return initiate_expected_impl<Value>(exec, std::move(factory),
			std::chrono::duration_cast<std::chrono::nanoseconds>(timed_token.time),
			std::move(timed_token.token)
		);
	}
	else
	{
		return initiate_expected_impl<Value>(exec, std::move(factory),
			std::chrono::nanoseconds::zero(), std::forward<Token>(token)
		);
	}
}

template <typename Factory>
[[nodiscard]] awaitable<sys_expected<std::monostate>> co_void_expected_value(Factory factory)
{
	auto result = co_await factory();
	if( not result )
		co_return sys_unexpected(result.error());
	co_return std::monostate {};
}

template <core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_void_impl
(const Exec &exec, Factory factory_fn,
	std::chrono::nanoseconds timeout, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code)>(
	[exec, operation = std::move(factory_fn), timeout]
	(auto completion_handler) mutable
	{
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto completion_exec = asio::get_associated_executor(
			completion_handler, exec
		);
		auto allocator = asio::get_associated_allocator(completion_handler);

		asio::co_spawn(exec,
			co_expected_with_timeout<std::monostate>(
				[void_operation = std::move(operation)]() mutable {
					return co_void_expected_value(std::move(void_operation));
				}, timeout
			),
			asio::bind_allocator(allocator, asio::bind_executor(completion_exec,
				asio::bind_cancellation_slot(slot,
				[handler = std::move(completion_handler)]
				(const std::exception_ptr &exception, sys_expected<std::monostate> result) mutable
				{
					if( auto error = exception_error(exception) )
						std::move(handler)(error);
					else if( not result )
						std::move(handler)(result.error());
					else
						std::move(handler)(error_code{});
				}))
			)
		);
	},
	completion_token);
}

template <core_concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_void(const Exec &exec, Factory factory, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		return initiate_expected_void_impl(exec, std::move(factory),
			std::chrono::duration_cast<std::chrono::nanoseconds>(timed_token.time),
			std::move(timed_token.token)
		);
	}
	else
	{
		return initiate_expected_void_impl(exec, std::move(factory),
			std::chrono::nanoseconds::zero(), std::forward<Token>(token)
		);
	}
}

// Normal completion tokens are passed straight to a compile-time composed
// operation. Only a positive redirect_time needs the coroutine timer race.
template <typename Value, core_concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_direct
(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,Value)>(
	[exec, start = std::move(initiation)](auto completion_handler) mutable
	{
		auto completion_exec = asio::get_associated_executor(
			completion_handler, exec
		);
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto allocator = asio::get_associated_allocator(completion_handler);

		auto bridge = asio::bind_allocator(allocator,
			asio::bind_executor(completion_exec, asio::bind_cancellation_slot(slot,
				[completion_exec, allocator,
				 handler = std::move(completion_handler)]
				(error_code error, Value value) mutable
				{
					asio::post(completion_exec, asio::bind_allocator(allocator,
					[final_handler = std::move(handler), error,
					 result_value = std::move(value)]() mutable
					{
						std::move(final_handler)(
							error, std::move(result_value)
						);
					}));
				})
			)
		);
		start(std::move(bridge));
	},
	completion_token);
}

template <core_concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_direct_void
(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code)>(
	[exec, start = std::move(initiation)](auto completion_handler) mutable
	{
		auto completion_exec = asio::get_associated_executor(
			completion_handler, exec
		);
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto allocator = asio::get_associated_allocator(completion_handler);

		auto bridge = asio::bind_allocator(allocator,
			asio::bind_executor(completion_exec, asio::bind_cancellation_slot(slot,
				[completion_exec, allocator,
				 handler = std::move(completion_handler)]
				(error_code error) mutable
				{
					asio::post(completion_exec, asio::bind_allocator(allocator,
					[final_handler = std::move(handler), error]() mutable {
						std::move(final_handler)(error);
					}));
				})
			)
		);
		start(std::move(bridge));
	},
	completion_token);
}

template <typename Value, core_concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io
(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		if( timed_token.time <= milliseconds::zero() )
		{
			return initiate_io_direct<Value>(exec, std::move(initiation),
				std::move(timed_token.token)
			);
		}
		return initiate_expected<Value>(exec,
		[operation = std::move(initiation)]() mutable
		-> awaitable<sys_expected<Value>>
		{
			error_code error {};
			auto value = co_await operation(
				asio::redirect_error(use_awaitable, error)
			);
			if( error )
				co_return sys_unexpected(error);
			co_return std::move(value);
		},
		std::move(timed_token));
	}
	else
	{
		return initiate_io_direct<Value>(exec, std::move(initiation),
			std::forward<Token>(token)
		);
	}
}

template <core_concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_void
(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		if( timed_token.time <= milliseconds::zero() )
		{
			return initiate_io_direct_void(exec, std::move(initiation),
				std::move(timed_token.token)
			);
		}
		return initiate_expected_void(exec,
		[operation = std::move(initiation)]() mutable -> awaitable<sys_expected<>>
		{
			error_code error {};
			co_await operation(asio::redirect_error(use_awaitable, error));
			if( error )
				co_return sys_unexpected(error);
			co_return make_sys_expected();
		},
		std::move(timed_token));
	}
	else
	{
		return initiate_io_direct_void(exec, std::move(initiation),
			std::forward<Token>(token)
		);
	}
}

} //namespace libgs::http::detail


#endif //LIBGS_HTTP_UTILS_DETAIL_ASYNC_EXPECTED_H
