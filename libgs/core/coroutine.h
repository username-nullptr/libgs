
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

#include <libgs/core/global.h>
#include <asio/experimental/awaitable_operators.hpp>

namespace libgs
{

template <typename T>
using awaitable = asio::awaitable<T>;

template <typename Func, typename Exec = asio::io_context>
auto co_spawn_detached(Func &&func, Exec &exec = io_context());

constexpr auto use_awaitable = asio::use_awaitable;

struct uawaitable_t
{
	auto operator[](asio::error_code &error) const noexcept {
		return asio::redirect_error(asio::use_awaitable, error);
	}
};

constexpr uawaitable_t use_awaitable_e;

template <typename Func, typename...Args>
concept awaitable_function = requires(Func &&func, Args&&...args) {
	func(std::forward<Args>(args)...);
};

template <typename Exec, typename Func, typename...Args>
auto co_post(Exec &exec, Func &&func, Args&&...args) requires awaitable_function<Func,Args...>;

template <typename Exec, typename Func, typename...Args>
auto co_dispatch(Exec &exec, Func &&func, Args&&...args) requires awaitable_function<Func,Args...>;

template <typename Func, typename...Args>
auto co_thread(Func &&func, Args&&...args) requires awaitable_function<Func,Args...>;

template<typename Rep, typename Period, typename Exec = asio::io_context>
awaitable<asio::error_code> co_sleep_for(const std::chrono::duration<Rep,Period> &rtime, Exec &exec = io_context());

template<typename Clock, typename Duration, typename Exec = asio::io_context>
awaitable<asio::error_code> co_sleep_until(const std::chrono::time_point<Clock,Duration> &atime, Exec &exec = io_context());

} //namespace libgs
#include <libgs/core/detail/coroutine.h>


#endif //LIBGS_CORE_COROUTINE_H
