// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_UTILS_H
#define LIBGS_CORO_UTILS_H

#include <libgs/coro/global.h>

#ifdef LIBGS_USING_BOOST_ASIO
# include <boost/asio/experimental/awaitable_operators.hpp>
# include <boost/asio/spawn.hpp>
#else
# include <asio/experimental/awaitable_operators.hpp>
#endif //LIBGS_USING_BOOST_ASIO

using namespace asio::experimental::awaitable_operators;

namespace libgs::coro
{

template <typename Rep, typename Period, concepts::sleep_opt_token Token = const use_awaitable_t&>
[[nodiscard]] LIBGS_CORO_TAPI auto sleep_for (
	concepts::sched auto &&exec, duration<Rep,Period> rtime, Token &&token = use_awaitable
);

template <typename Rep, typename Period, concepts::sleep_opt_token Token = const use_awaitable_t&>
[[nodiscard]] LIBGS_CORO_TAPI auto sleep_for (
	duration<Rep,Period> rtime, Token &&token = use_awaitable
);

template <typename Clock, typename Duration, concepts::sleep_opt_token Token = const use_awaitable_t&>
[[nodiscard]] LIBGS_CORO_TAPI auto sleep_until (
	concepts::sched auto &&exec, time_point<Clock,Duration> atime, Token &&token = use_awaitable
);

template <typename Clock, typename Duration, concepts::sleep_opt_token Token = const use_awaitable_t&>
[[nodiscard]] LIBGS_CORO_TAPI auto sleep_until (
	time_point<Clock,Duration> atime, Token &&token = use_awaitable
);

template <typename T>
[[nodiscard]] LIBGS_CORO_TAPI
awaitable<T> wait(const std::future<T> &future);

[[nodiscard]] LIBGS_CORO_API
awaitable<void> wait(const asio::thread_pool &pool);

[[nodiscard]] LIBGS_CORO_API
awaitable<void> wait (const std::thread &thread);

[[nodiscard]] LIBGS_CORO_API
awaitable<void> wait(const jthread &thread);

template <concepts::sched Exec = io_executor_t>
[[nodiscard]] LIBGS_CORO_TAPI
awaitable<asio::any_io_executor> goto_exec(Exec &&exec = get_executor());

[[nodiscard]] LIBGS_CORO_API
awaitable<asio::any_io_executor> goto_thread();

template <concepts::any_async_tf_opt_token Token>
LIBGS_CORO_TAPI bool check_error (
	Token &token, const error_code &error, const char *message = nullptr
) requires (not std::is_const_v<Token>);

#ifdef LIBGS_USING_BOOST_ASIO

template<typename Rep, typename Period, concepts::exec YCExec, concepts::sched Exec = YCExec>
[[nodiscard]] LIBGS_CORO_TAPI error_code sleep_for (
	const std::chrono::duration<Rep,Period> &rtime, basic_yield_context<Exec> yc, Exec &&exec = yc.get_executor()
);

template<typename Clock, typename Duration, concepts::exec YCExec, concepts::sched Exec = YCExec>
[[nodiscard]] LIBGS_CORO_TAPI error_code sleep_until (
	const std::chrono::time_point<Clock,Duration> &atime, yield_context yc, Exec &&exec = yc.get_executor()
);

template <typename T, concepts::exec YCExec>
[[nodiscard]] LIBGS_CORO_TAPI T wait(basic_yield_context<YCExec> yc, const std::future<T> &future);

template <concepts::exec YCExec>
[[nodiscard]] LIBGS_CORO_VAPI void wait(basic_yield_context<YCExec> yc, const asio::thread_pool &pool);

template <concepts::exec YCExec>
[[nodiscard]] LIBGS_CORO_VAPI void wait(basic_yield_context<YCExec> yc, const std::thread &thread);

template <concepts::exec YCExec>
[[nodiscard]] LIBGS_CORO_VAPI void wait(basic_yield_context<YCExec> yc, const jthread &thread);

template <concepts::exec YCExec, concepts::sched Exec = YCExec>
[[nodiscard]] LIBGS_CORO_TAPI asio::any_io_executor goto_exec (
	basic_yield_context<YCExec> yc, Exec &&exec = yc.get_executor()
);

template <concepts::exec YCExec>
[[nodiscard]] LIBGS_CORO_VAPI asio::any_io_executor goto_thread (
	basic_yield_context<YCExec> yc
);

template <concepts::exec Exec>
LIBGS_CORO_VAPI bool check_error (
	basic_yield_context<Exec> &yc, const error_code &error, const char *message = nullptr
);

#endif //LIBGS_USING_BOOST_ASIO

namespace literals
{

[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_y  (unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_mon(unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_d  (unsigned long long value);

[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_h  (unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_min(unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_s  (unsigned long long value);

[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_ms (unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_us (unsigned long long value);
[[nodiscard]] LIBGS_CORO_API awaitable<error_code> operator""_ns (unsigned long long value);

}} //namespace libgs::coro::literals
#include <libgs/coro/detail/utils.h>


#endif //LIBGS_CORO_UTILS_H
