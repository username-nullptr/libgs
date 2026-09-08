// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_DETAIL_ASYNC_EXPECTED_H
#define LIBGS_CORE_DETAIL_ASYNC_EXPECTED_H

namespace libgs { namespace detail
{

[[nodiscard]] inline error_code canonical_error(error_code error) noexcept
{
	if( not error )
		return {};

	std::string_view category_name(error.category().name());
	auto category_matches = [category_name](const error_code &sample) noexcept {
		return category_name == sample.category().name();
	};
	if( category_matches(asio::error::make_error_code(asio::error::operation_aborted)) )
	{
		return asio::error::make_error_code(
			static_cast<asio::error::basic_errors>(error.value())
		);
	}
	if( category_matches(asio::error::make_error_code(asio::error::eof)) )
	{
		return asio::error::make_error_code (
			static_cast<asio::error::misc_errors>(error.value())
		);
	}
	if( category_matches(asio::error::make_error_code(asio::error::host_not_found)) )
	{
		return asio::error::make_error_code (
			static_cast<asio::error::netdb_errors>(error.value())
		);
	}
	if( category_matches(asio::error::make_error_code(asio::error::service_not_found)) )
	{
		return asio::error::make_error_code (
			static_cast<asio::error::addrinfo_errors>(error.value())
		);
	}
	if( category_name == std::generic_category().name() )
		return {error.value(), std::generic_category()};
	if( category_name == std::system_category().name() )
		return {error.value(), std::system_category()};
	return error;
}

template <typename Value>
void canonicalize_expected(sys_expected<Value> &expected) noexcept
{
	if( not expected )
		expected = sys_unexpected(canonical_error(expected.error()));
}

template <typename>
struct is_async_argument_reference : std::false_type {};

template <typename T>
struct is_async_argument_reference<std::reference_wrapper<T>> : std::true_type {};

template <typename Value, typename Factory>
[[nodiscard]] awaitable<sys_expected<Value>>
co_expected_with_timeout(Factory factory, std::chrono::nanoseconds timeout)
{
	if( timeout <= std::chrono::nanoseconds::zero() )
		co_return co_await factory();

	using namespace asio::experimental::awaitable_operators;
	auto exec = co_await asio::this_coro::executor;

	// operator|| cancels the unfinished operation. The I/O side receives that
	// cancellation through its associated slot before this coroutine returns.
	auto result = co_await (
		factory() or libgs::sleep_for(exec, timeout)
	);
	if( result.index() == 0 )
		co_return std::move(std::get<0>(result));

	if( const auto &timer_error = std::get<1>(result) )
		co_return sys_unexpected(timer_error);
	co_return sys_unexpected(make_error_code(errc::timed_out));
}

template <typename Value, concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected
(const Exec &exec, Factory factory_fn, std::chrono::nanoseconds timeout, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,Value)>(
	[exec, operation = std::move(factory_fn), timeout](auto completion_handler) mutable
	{
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto completion_exec = asio::get_associated_executor (
			completion_handler, exec
		);
		auto allocator = asio::get_associated_allocator(completion_handler);
		asio::co_spawn(exec,
			co_expected_with_timeout<Value>(std::move(operation), timeout),
			asio::bind_allocator(allocator, asio::bind_executor(completion_exec,
				asio::bind_cancellation_slot(slot, [handler = std::move(completion_handler)]
				(const std::exception_ptr &exception, sys_expected<Value> result) mutable
				{
					if( auto error = exception_error(exception) )
					{
						std::move(handler)(error, Value{});
						return ;
					}
					canonicalize_expected(result);
					if( not result )
						std::move(handler)(result.error(), Value{});
					else
						std::move(handler)(error_code{}, std::move(*result));
				})
			))
		);
	},
	completion_token);
}

template <typename>
struct token_has_redirect_error : std::false_type {};

template <typename Token>
struct token_has_redirect_error<redirect_error_t<Token>> : std::true_type {};

template <typename Token>
struct token_has_redirect_error<redirect_time_t<Token>> :
	token_has_redirect_error<Token> {};

template <typename Token, typename CancellationSlot>
struct token_has_redirect_error<cancellation_slot_binder<Token,CancellationSlot>> :
	token_has_redirect_error<Token> {};

template <typename Token>
constexpr bool token_has_redirect_error_v =
	token_has_redirect_error<std::remove_cvref_t<Token>>::value;

// Some APIs deliberately keep expected in their future/coroutine result.  This
// bridge still presents an error_code to redirect_error, but never turns that
// error into an exception for an unredirected future or awaitable.
template <typename Value, concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_preserved_expected
(const Exec &exec, Factory factory_fn, std::chrono::nanoseconds timeout, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	if constexpr( token_has_redirect_error_v<token_t> )
	{
		return asio::async_initiate<token_t,void(error_code,sys_expected<Value>)>(
		[exec, operation = std::move(factory_fn), timeout](auto completion_handler) mutable
		{
			auto slot = asio::get_associated_cancellation_slot(completion_handler);
			auto completion_exec = asio::get_associated_executor (
				completion_handler, exec
			);
			auto allocator = asio::get_associated_allocator(completion_handler);
			asio::co_spawn(exec,
				co_expected_with_timeout<Value>(std::move(operation), timeout),
				asio::bind_allocator(allocator, asio::bind_executor(completion_exec,
					asio::bind_cancellation_slot(slot, [handler = std::move(completion_handler)]
					(const std::exception_ptr &exception, sys_expected<Value> result) mutable
					{
						if( auto error = exception_error(exception) )
						{
							std::move(handler)(error,
								sys_expected<Value>(sys_unexpected(error))
							);
							return ;
						}
						canonicalize_expected(result);
						const auto error = result ? error_code{} : result.error();
						std::move(handler)(error, std::move(result));
					})
				))
			);
		},
		completion_token);
	}
	else
	{
		return asio::async_initiate<token_t,void(sys_expected<Value>)>(
		[exec, operation = std::move(factory_fn), timeout](auto completion_handler) mutable
		{
			auto slot = asio::get_associated_cancellation_slot(completion_handler);
			auto completion_exec = asio::get_associated_executor (
				completion_handler, exec
			);
			auto allocator = asio::get_associated_allocator(completion_handler);
			asio::co_spawn(exec,
				co_expected_with_timeout<Value>(std::move(operation), timeout),
				asio::bind_allocator(allocator, asio::bind_executor(completion_exec,
					asio::bind_cancellation_slot(slot, [handler = std::move(completion_handler)]
					(const std::exception_ptr &exception, sys_expected<Value> result) mutable
					{
						if( auto error = exception_error(exception) )
						{
							std::move(handler)(sys_expected<Value>(
								sys_unexpected(error)
							));
							return ;
						}
						canonicalize_expected(result);
						std::move(handler)(std::move(result));
					})
				))
			);
		},
		completion_token);
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

template <concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_void
(const Exec &exec, Factory factory_fn, std::chrono::nanoseconds timeout, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code)>(
	[exec, operation = std::move(factory_fn), timeout](auto completion_handler) mutable
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
				asio::bind_cancellation_slot(slot, [handler = std::move(completion_handler)]
				(const std::exception_ptr &exception, sys_expected<std::monostate> result) mutable
				{
					if( auto error = exception_error(exception) )
						std::move(handler)(error);
					else if( not result )
						std::move(handler)(canonical_error(result.error()));
					else
						std::move(handler)(error_code{});
				})
			))
		);
	},
	completion_token);
}

// Ordinary completion tokens stay on the direct async_initiate path. Only a
// positive redirect_time creates the coroutine/timer race above.
template <typename Value, concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_direct(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code,Value)>(
	[exec, start = std::move(initiation)](auto completion_handler) mutable
	{
		auto completion_exec = asio::get_associated_executor (
			completion_handler, exec
		);
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto allocator = asio::get_associated_allocator(completion_handler);

		auto bridge = asio::bind_allocator(allocator,
			asio::bind_executor(completion_exec, asio::bind_cancellation_slot(slot,
				[completion_exec, allocator, handler = std::move(completion_handler)]
				(error_code error, Value result_value) mutable
				{
					error = canonical_error(error);
					asio::post(completion_exec, asio::bind_allocator(allocator, [
						final_handler = std::move(handler), error,
						final_value = std::move(result_value)
					]() mutable {
						std::move(final_handler)(error, std::move(final_value));
					}));
				})
			)
		);
		start(std::move(bridge));
	},
	completion_token);
}

template <concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_direct_void(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));

	return asio::async_initiate<token_t,void(error_code)>(
	[exec, start = std::move(initiation)](auto completion_handler) mutable
	{
		auto completion_exec = asio::get_associated_executor (
			completion_handler, exec
		);
		auto slot = asio::get_associated_cancellation_slot(completion_handler);
		auto allocator = asio::get_associated_allocator(completion_handler);

		auto bridge = asio::bind_allocator(allocator,
			asio::bind_executor(completion_exec, asio::bind_cancellation_slot(slot,
			[completion_exec, allocator, handler = std::move(completion_handler)](error_code error) mutable
			{
				error = canonical_error(error);
				asio::post(completion_exec, asio::bind_allocator(allocator,
				[final_handler = std::move(handler), error]() mutable {
					std::move(final_handler)(error);
				}));
			}))
		);
		start(std::move(bridge));
	},
	completion_token);
}

} //namespace detail

template <typename Buffer, typename Source>
Buffer copy_buffer_data(Source &&source) requires
(is_buffer_v<Buffer> and is_buffer_v<std::remove_cvref_t<Source>> and not is_array_buffer_v<Buffer>)
{
	using source_t = std::remove_cvref_t<Source>;
	if constexpr( std::same_as<Buffer,source_t> )
		return std::forward<Source>(source);
	else
	{
		using value_t = Buffer::value_type;
		const auto byte_size = source.size() *
			sizeof(typename source_t::value_type);

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
		system_error::loc_throw(detail::canonical_error(expected.error()));
	return std::move(*expected);
}

template <typename Value>
[[nodiscard]] Value expected_value_or_error(sys_expected<Value> expected, error_code &error)
	noexcept(std::is_nothrow_move_constructible_v<Value>)
{
	if( not expected )
	{
		error = detail::canonical_error(expected.error());
		return {};
	}
	error.clear();
	return std::move(*expected);
}

template <typename T>
[[nodiscard]] auto capture_async_argument(T &&argument)
{
	if constexpr( std::is_lvalue_reference_v<T> )
		return std::ref(argument);
	else
		return std::remove_cvref_t<T>(std::forward<T>(argument));
}

template <typename T>
[[nodiscard]] decltype(auto) unwrap_async_argument(T &argument) noexcept
{
	if constexpr( detail::is_async_argument_reference<std::remove_cvref_t<T>>::value )
		return argument.get();
	else
		return (argument);
}

inline error_code exception_error(const std::exception_ptr &exception) noexcept
{
	if( not exception )
		return {};
	try {
		std::rethrow_exception(exception);
	}
	catch(const std::system_error &ex) {
		return detail::canonical_error(ex.code());
	}
	catch(const std::bad_alloc&) {
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {}
	return make_error_code(std::errc::io_error);
}

template <typename Value, concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected(const Exec &exec, Factory factory, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		return detail::initiate_expected<Value>(exec, std::move(factory),
			std::chrono::duration_cast<std::chrono::nanoseconds>(timed_token.time),
			std::move(timed_token.token)
		);
	}
	else
	{
		return detail::initiate_expected<Value>(exec, std::move(factory),
			std::chrono::nanoseconds::zero(), std::forward<Token>(token)
		);
	}
}

template <typename Value, concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_preserved_expected
(const Exec &exec, Factory factory, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		return detail::initiate_preserved_expected<Value>(exec, std::move(factory),
			std::chrono::duration_cast<std::chrono::nanoseconds>(timed_token.time),
			std::move(timed_token.token)
		);
	}
	else
	{
		return detail::initiate_preserved_expected<Value>(exec, std::move(factory),
			std::chrono::nanoseconds::zero(), std::forward<Token>(token)
		);
	}
}

template <concepts::exec Exec, typename Factory, typename Token>
[[nodiscard]] auto initiate_expected_void(const Exec &exec, Factory factory, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		return detail::initiate_expected_void(exec, std::move(factory),
			std::chrono::duration_cast<std::chrono::nanoseconds>(timed_token.time),
			std::move(timed_token.token)
		);
	}
	else
	{
		return detail::initiate_expected_void(exec, std::move(factory),
			std::chrono::nanoseconds::zero(), std::forward<Token>(token)
		);
	}
}

template <typename Value, concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		if( timed_token.time <= milliseconds::zero() )
		{
			return detail::initiate_io_direct<Value>(exec, std::move(initiation),
				std::move(timed_token.token)
			);
		}
		return initiate_expected<Value>(exec,
		[operation = std::move(initiation)]() mutable -> awaitable<sys_expected<Value>>
		{
			error_code error {};
			auto result_value = co_await operation(
				asio::redirect_error(use_awaitable, error)
			);
			if( error )
				co_return sys_unexpected(error);
			co_return std::move(result_value);
		},
		std::move(timed_token));
	}
	else
	{
		return detail::initiate_io_direct<Value>(exec, std::move(initiation),
			std::forward<Token>(token)
		);
	}
}

template <concepts::exec Exec, typename Initiator, typename Token>
[[nodiscard]] auto initiate_io_void(const Exec &exec, Initiator initiation, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_redirect_time_v<token_t> )
	{
		token_t timed_token(std::forward<Token>(token));
		if( timed_token.time <= milliseconds::zero() )
		{
			return detail::initiate_io_direct_void(exec, std::move(initiation),
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
		return detail::initiate_io_direct_void(exec, std::move(initiation),
			std::forward<Token>(token)
		);
	}
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_ASYNC_EXPECTED_H
