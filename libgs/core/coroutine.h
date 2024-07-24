
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

template <typename T>
using awaitable = asio::awaitable<T>;

template <concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn(concept_awaitable_function auto &&func, Exec &exec = execution::io_context());

template <typename T, concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn(awaitable<T> &&a, Exec &exec = execution::io_context());

template <concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn_detached(concept_awaitable_function auto &&func, Exec &exec = execution::io_context());

template <typename T, concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn_detached(awaitable<T> &&a, Exec &exec = execution::io_context());

template <concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn_future(concept_awaitable_function auto &&func, Exec &exec = execution::io_context());

template <typename T, concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI auto co_spawn_future(awaitable<T> &&a, Exec &exec = execution::io_context());

constexpr auto use_awaitable = asio::use_awaitable;

using use_awaitable_t = decltype(use_awaitable);

template <typename Exec, typename Func>
LIBGS_CORE_TAPI auto co_post(Exec &exec, Func &&func) requires concept_callable<Func>;

template <typename Exec, typename Func>
LIBGS_CORE_TAPI auto co_dispatch(Exec &exec, Func &&func) requires concept_callable<Func>;

template <typename Func>
LIBGS_CORE_TAPI auto co_thread(Func &&func) requires concept_callable<Func>;

template<typename Rep, typename Period, typename Exec = asio::io_context>
LIBGS_CORE_TAPI awaitable<error_code> co_sleep_for
(const std::chrono::duration<Rep,Period> &rtime, Exec &exec = execution::io_context());

template<typename Clock, typename Duration, typename Exec = asio::io_context>
LIBGS_CORE_TAPI awaitable<error_code> co_sleep_until
(const std::chrono::time_point<Clock,Duration> &atime, Exec &exec = execution::io_context());

template <typename T>
LIBGS_CORE_TAPI awaitable<T> co_wait(const std::future<T> &future);

LIBGS_CORE_VAPI awaitable<void> co_wait(const asio::thread_pool &pool);
LIBGS_CORE_VAPI awaitable<void> co_wait(const std::thread &thread);

template <concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI awaitable<void> co_to_exec(Exec &exec = execution::io_context());

LIBGS_CORE_VAPI awaitable<void> co_to_thread();

#ifdef LIBGS_USING_BOOST_ASIO

using yield_context = asio::yield_context;

template <typename Exec, typename Func>
LIBGS_CORE_TAPI auto co_post(Exec &exec, yield_context &yc, Func &&func) requires concept_callable<Func>;

template <typename Exec, typename Func>
LIBGS_CORE_TAPI auto co_dispatch(Exec &exec, yield_context &yc, Func &&func) requires concept_callable<Func>;

template <typename Func>
LIBGS_CORE_TAPI auto co_thread(yield_context &yc, Func &&func) requires concept_callable<Func>;

template<typename Rep, typename Period, typename Exec = asio::io_context>
LIBGS_CORE_TAPI error_code co_sleep_for
(const std::chrono::duration<Rep,Period> &rtime, yield_context &yc, Exec &exec = execution::io_context());

template<typename Clock, typename Duration, typename Exec = asio::io_context>
LIBGS_CORE_TAPI error_code co_sleep_until
(const std::chrono::time_point<Clock,Duration> &atime, yield_context &yc, Exec &exec = execution::io_context());

template <typename T>
LIBGS_CORE_TAPI T co_wait(yield_context &yc, const std::future<T> &future);

LIBGS_CORE_VAPI void co_wait(yield_context &yc, const asio::thread_pool &pool);
LIBGS_CORE_VAPI void co_wait(yield_context &yc, const std::thread &thread);

template <concept_schedulable Exec = asio::io_context>
LIBGS_CORE_TAPI void co_to_exec(yield_context &yc, Exec &exec = execution::io_context());

LIBGS_CORE_VAPI void co_to_thread(yield_context &yc);

#endif //LIBGS_USING_BOOST_ASIO

} //namespace libgs
#include <libgs/core/detail/coroutine.h>


#endif //LIBGS_CORE_COROUTINE_H
