
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
#include <asio/experimental/awaitable_operators.hpp>

using namespace  asio::experimental::awaitable_operators;

namespace libgs
{

template <typename T>
using awaitable = asio::awaitable<T>;

template <concept_schedulable Exec = asio::io_context>
auto co_spawn_detached(concept_awaitable_function auto &&func, Exec &exec = io_context());

template <typename T, concept_schedulable Exec = asio::io_context>
auto co_spawn_detached(awaitable<T> &&a, Exec &exec = io_context());

template <concept_schedulable Exec = asio::io_context>
auto co_spawn_future(concept_awaitable_function auto &&func, Exec &exec = io_context());

template <typename T, concept_schedulable Exec = asio::io_context>
auto co_spawn_future(awaitable<T> &&a, Exec &exec = io_context());

constexpr auto use_awaitable = asio::use_awaitable;

using use_awaitable_t = decltype(use_awaitable);

struct use_awaitable_e_t
{
	auto operator[](error_code &error) const noexcept {
		return redirect_error(asio::use_awaitable, error);
	}
};
constexpr use_awaitable_e_t use_awaitable_e;

using ua_redirect_error_t = typename function_traits<decltype(asio::redirect_error<use_awaitable_t>)>::return_type;

template <typename Exec, typename Func, typename...Args>
auto co_post(Exec &exec, Func &&func, Args&&...args) requires concept_callable<Func,Args...>;

template <typename Exec, typename Func, typename...Args>
auto co_dispatch(Exec &exec, Func &&func, Args&&...args) requires concept_callable<Func,Args...>;

template <typename Func, typename...Args>
auto co_thread(Func &&func, Args&&...args) requires concept_callable<Func,Args...>;

template<typename Rep, typename Period, typename Exec = asio::io_context>
awaitable<error_code> co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, Exec &exec = io_context());

template<typename Clock, typename Duration, typename Exec = asio::io_context>
awaitable<error_code> co_sleep_until(const std::chrono::time_point<Clock,Duration> &atime, Exec &exec = io_context());

template <typename T>
awaitable<T> co_wait(const std::future<T> &future);

LIBGS_CORE_VAPI awaitable<void> co_wait(const asio::thread_pool &pool);
LIBGS_CORE_VAPI awaitable<void> co_wait(const std::thread &thread);

} //namespace libgs
#include <libgs/core/detail/coroutine.h>


#endif //LIBGS_CORE_COROUTINE_H
