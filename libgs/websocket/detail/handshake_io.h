// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_HANDSHAKE_IO_H
#define LIBGS_WEBSOCKET_DETAIL_HANDSHAKE_IO_H

#include <libgs/core/async_expected.h>

namespace libgs::websocket::detail
{

template <typename Value>
struct handshake_io_completion
{
	error_code error;
	Value value;
};

template <typename Value, typename Initiator>
[[nodiscard]] awaitable<std::unique_ptr<handshake_io_completion<Value>>>
co_handshake_io_with_timeout(Initiator initiation,
	std::chrono::nanoseconds timeout)
{
	auto exec = co_await asio::this_coro::executor;
	asio::steady_timer timer(exec);
	timer.expires_after(timeout);

	deferred_t completion_token;
	auto io_operation = asio::async_initiate<
		deferred_t,void(error_code,Value)>(
		[start = std::move(initiation)](auto completion_handler) mutable {
			start(std::move(completion_handler));
		}, completion_token);
	auto [order, io_error, value, timer_error] =
		co_await asio::experimental::make_parallel_group(
			std::move(io_operation), timer.async_wait(deferred)
		).async_wait(asio::experimental::wait_for_one(), use_awaitable);

	io_error = libgs::detail::canonical_error(io_error);
	timer_error = libgs::detail::canonical_error(timer_error);
	if(order[0] == 0)
		co_return std::make_unique<handshake_io_completion<Value>>(
			handshake_io_completion<Value>{io_error, std::move(value)});
	if(not timer_error)
	{
		co_return std::make_unique<handshake_io_completion<Value>>(
			handshake_io_completion<Value>{
				asio::error::timed_out, std::move(value)});
	}
	co_return std::make_unique<handshake_io_completion<Value>>(
		handshake_io_completion<Value>{
			io_error ? io_error : timer_error, std::move(value)});
}

// The generic timed I/O bridge uses Value{} for an exceptional completion.
// Handshake results deliberately have no executor-free default constructor, so
// this variant receives a factory for an idle result bound to the right executor.
template <typename Value, core_concepts::exec Exec, typename Initiator,
	typename Fallback, typename Token>
[[nodiscard]] auto initiate_handshake_io(const Exec &exec,
	Initiator initiation, std::chrono::milliseconds timeout,
	Fallback fallback, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	token_t completion_token(std::forward<Token>(token));
	const auto effective_timeout = timeout > std::chrono::milliseconds::zero() ?
		std::chrono::duration_cast<std::chrono::nanoseconds>(timeout) :
		std::chrono::nanoseconds(1);

	return asio::async_initiate<token_t,void(error_code,Value)>(
		[exec, operation = std::move(initiation), effective_timeout,
		 make_fallback = std::move(fallback)](auto completion_handler) mutable
		{
			auto slot = asio::get_associated_cancellation_slot(completion_handler);
			auto completion_exec = asio::get_associated_executor(
				completion_handler, exec);
			auto allocator = asio::get_associated_allocator(completion_handler);

			asio::co_spawn(exec,
				co_handshake_io_with_timeout<Value>(
					std::move(operation), effective_timeout),
				asio::bind_allocator(allocator,
					asio::bind_executor(completion_exec,
						asio::bind_cancellation_slot(slot,
						[handler = std::move(completion_handler),
						 make_idle = std::move(make_fallback)](
							const std::exception_ptr &exception,
							std::unique_ptr<handshake_io_completion<Value>> result) mutable
						{
							if(auto error = exception_error(exception))
								std::move(handler)(error, make_idle());
							else if(not result)
								std::move(handler)(
									make_error_code(std::errc::io_error), make_idle());
							else
								std::move(handler)(result->error,
									std::move(result->value));
						})))
			);
		}, completion_token);
}

} //namespace libgs::websocket::detail

#endif //LIBGS_WEBSOCKET_DETAIL_HANDSHAKE_IO_H
