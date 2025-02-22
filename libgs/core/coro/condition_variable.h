
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

#ifndef LIBGS_CORE_CORO_CONDITION_VARIABLE_H
#define LIBGS_CORE_CORO_CONDITION_VARIABLE_H

#include <libgs/core/coro/mutex.h>
#include <condition_variable>

namespace libgs
{

// TODO ... ...
class LIBGS_CORE_VAPI co_condition_variable
{
	LIBGS_DISABLE_COPY_MOVE(co_condition_variable)

public:
	co_condition_variable();
	~co_condition_variable() noexcept(false);

public:
	template <typename Mutex>
    [[nodiscard]] awaitable<void> wait(co_unique_lock<Mutex> &lock) noexcept;

    template <typename Mutex>
    [[nodiscard]] awaitable<void> wait(co_unique_lock<Mutex> &lock, auto pred);

	template <typename Mutex>
    [[nodiscard]] awaitable<void> wait (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock
    ) noexcept;

    template <typename Mutex>
    [[nodiscard]] awaitable<void> wait (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock, auto pred
    );

    void notify_one() noexcept;
    void notify_all() noexcept;

public:
    template <typename Mutex, typename Rep, typename Period>
    [[nodiscard]] awaitable<bool> wait_for (
    	co_unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime
    );
    template <typename Mutex, typename Rep, typename Period>
    [[nodiscard]] awaitable<bool> wait_for (
    	co_unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime, auto pred
    );
    template <typename Mutex, typename Clock, typename Duration>
    [[nodiscard]] awaitable<bool> wait_until (
    	co_unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime
    );
    template <typename Mutex, typename Clock, typename Duration>
    [[nodiscard]] awaitable<bool> wait_until (
    	co_unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime, auto pred
    );

public:
    template <typename Mutex, typename Rep, typename Period>
    [[nodiscard]] awaitable<bool> wait_for (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock,
    	const duration<Rep,Period> &rtime
    );
    template <typename Mutex, typename Rep, typename Period>
    [[nodiscard]] awaitable<bool> wait_for (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock,
    	const duration<Rep,Period> &rtime, auto pred
    );
    template <typename Mutex, typename Clock, typename Duration>
    [[nodiscard]] awaitable<bool> wait_until (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock,
    	const time_point<Clock,Duration> &atime
    );
    template <typename Mutex, typename Clock, typename Duration>
    [[nodiscard]] awaitable<bool> wait_until (
    	concepts::schedulable auto &&exec, co_unique_lock<Mutex> &lock,
    	const time_point<Clock,Duration> &atime, auto pred
    );

private:
	class impl;
	impl *m_impl;

	std::condition_variable aaa;
	std::condition_variable_any bbb;
};

} //namespace libgs
#include <libgs/core/coro/detail/condition_variable.h>


#endif //LIBGS_CORE_CORO_CONDITION_VARIABLE_H