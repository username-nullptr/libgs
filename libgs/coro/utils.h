
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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
[[nodiscard]] LIBGS_CORO_TAPI awaitable<T> wait (
	const std::future<T> &future
);

[[nodiscard]] LIBGS_CORO_VAPI awaitable<void> wait (
	const asio::thread_pool &pool
);

[[nodiscard]] LIBGS_CORO_VAPI awaitable<void> wait (
	const std::thread &thread
);

template <concepts::sched Exec = io_executor_t>
[[nodiscard]] LIBGS_CORO_TAPI awaitable<asio::any_io_executor> goto_exec (
	Exec &&exec = get_executor()
);

[[nodiscard]] LIBGS_CORO_VAPI awaitable<asio::any_io_executor> goto_thread();

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

[[nodiscard]] LIBGS_CORO_VAPI auto operator""_s (unsigned long long value);
[[nodiscard]] LIBGS_CORO_VAPI auto operator""_ms(unsigned long long value);
[[nodiscard]] LIBGS_CORO_VAPI auto operator""_us(unsigned long long value);
[[nodiscard]] LIBGS_CORO_VAPI auto operator""_ns(unsigned long long value);

}} //namespace libgs::coro::literals
#include <libgs/coro/detail/utils.h>


#endif //LIBGS_CORO_UTILS_H
