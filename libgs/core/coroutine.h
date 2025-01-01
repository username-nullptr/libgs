
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

#ifndef LIBGS_CORE_COROUTINE_H
#define LIBGS_CORE_COROUTINE_H

#include <libgs/core/execution.h>

#ifdef LIBGS_USING_BOOST_ASIO
# include <boost/asio/experimental/awaitable_operators.hpp>
# include <boost/asio/spawn.hpp>
#else
# include <asio/experimental/awaitable_operators.hpp>
#endif //LIBGS_USING_BOOST_ASIO

using namespace asio::experimental::awaitable_operators;

namespace libgs
{

template <typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<error_code> co_sleep_for (
	const std::chrono::duration<Rep,Period> &rtime, concepts::schedulable auto &&exec
);

template <typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<error_code> co_sleep_for (
	const std::chrono::duration<Rep,Period> &rtime
);

template <typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<error_code> co_sleep_until (
	const std::chrono::time_point<Rep,Period> &atime, concepts::schedulable auto &&exec
);

template <typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<error_code> co_sleep_until (
	const std::chrono::time_point<Rep,Period> &atime
);

template <typename T>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<T> co_wait (
	const std::future<T> &future
);

[[nodiscard]] LIBGS_CORE_VAPI awaitable<void> co_wait (
	const asio::thread_pool &pool
);

[[nodiscard]] LIBGS_CORE_VAPI awaitable<void> co_wait (
	const std::thread &thread
);

template <concepts::schedulable Exec = io_executor_t>
[[nodiscard]] LIBGS_CORE_TAPI awaitable<asio::any_io_executor> co_to_exec (
	Exec &&exec = get_executor()
);

[[nodiscard]] LIBGS_CORE_VAPI awaitable<asio::any_io_executor> co_to_thread();

template <concepts::any_async_tf_opt_token Token>
LIBGS_CORE_TAPI bool check_error (
	Token &token, const error_code &error, const char *message = nullptr
) requires (not std::is_const_v<Token>);

#ifdef LIBGS_USING_BOOST_ASIO

template <concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_TAPI auto co_post (
	concepts::schedulable auto &&exec, basic_yield_context<YCExec> yc, concepts::callable auto &&func
);

template <concepts::execution YCExec>
LIBGS_CORE_TAPI auto co_dispatch (
	concepts::schedulable auto &&exec, basic_yield_context<YCExec> yc, concepts::callable auto &&func
);

template <concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_TAPI auto co_thread (
	basic_yield_context<YCExec> yc, concepts::callable auto &&func
);

template<typename Rep, typename Period, concepts::execution YCExec, concepts::schedulable Exec = YCExec>
[[nodiscard]] LIBGS_CORE_TAPI error_code co_sleep_for (
	const std::chrono::duration<Rep,Period> &rtime, basic_yield_context<Exec> yc, Exec &&exec = yc.get_executor()
);

template<typename Clock, typename Duration, concepts::execution YCExec, concepts::schedulable Exec = YCExec>
[[nodiscard]] LIBGS_CORE_TAPI error_code co_sleep_until (
	const std::chrono::time_point<Clock,Duration> &atime, yield_context yc, Exec &&exec = yc.get_executor()
);

template <typename T, concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_TAPI T co_wait(basic_yield_context<YCExec> yc, const std::future<T> &future);

template <concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_VAPI void co_wait(basic_yield_context<YCExec> yc, const asio::thread_pool &pool);

template <concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_VAPI void co_wait(basic_yield_context<YCExec> yc, const std::thread &thread);

template <concepts::execution YCExec, concepts::schedulable Exec = YCExec>
[[nodiscard]] LIBGS_CORE_TAPI asio::any_io_executor co_to_exec (
	basic_yield_context<YCExec> yc, Exec &&exec = yc.get_executor()
);

template <concepts::execution YCExec>
[[nodiscard]] LIBGS_CORE_VAPI asio::any_io_executor co_to_thread (
	basic_yield_context<YCExec> yc
);

template <concepts::execution Exec>
LIBGS_CORE_VAPI bool check_error (
	basic_yield_context<Exec> &yc, const error_code &error, const char *message = nullptr
);

#endif //LIBGS_USING_BOOST_ASIO

} //namespace libgs
#include <libgs/core/detail/coroutine.h>


#endif //LIBGS_CORE_COROUTINE_H
