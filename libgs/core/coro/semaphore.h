
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

#ifndef LIBGS_CORE_CORO_SEMAPHORE_H
#define LIBGS_CORE_CORO_SEMAPHORE_H

#include <libgs/core/global.h>

namespace libgs
{

template<size_t Max = std::numeric_limits<size_t>::max()>
class LIBGS_CORE_TAPI co_basic_semaphore
{
	LIBGS_DISABLE_COPY_MOVE(co_basic_semaphore)
	constexpr static size_t max_v = Max;

	static_assert(max_v > 0,
		"Max must be greater than 0"
	);
public:
	explicit co_basic_semaphore(size_t initial_count = max_v);
	~co_basic_semaphore() noexcept(false);

public:
	[[nodiscard]] awaitable<void> acquire(concepts::schedulable auto &&exec);
	[[nodiscard]] awaitable<void> acquire();

	[[nodiscard]] bool try_acquire();
	size_t release(size_t n = 1) requires (max_v > 1);
	size_t release() requires (max_v == 1);

public:
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_acquire_for (
		concepts::schedulable auto &&exec, const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_acquire_until (
		concepts::schedulable auto &&exec, const time_point<Clock,Duration> &timeout
	);
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_acquire_for (
		const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_acquire_until (
		const time_point<Clock,Duration> &timeout
	);

public:
	[[nodiscard]] consteval size_t max() const noexcept;
	[[nodiscard]] size_t count() const noexcept;

private:
	class impl;
	impl *m_impl;
};

using co_binary_semaphore = co_basic_semaphore<1>;
using co_semaphore = co_basic_semaphore<>;

} //namespace libgs
#include <libgs/core/coro/detail/semaphore.h>


#endif //LIBGS_CORE_CORO_SEMAPHORE_H