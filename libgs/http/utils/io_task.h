
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

#ifndef LIBGS_HTTP_UTILS_IO_TASK_H
#define LIBGS_HTTP_UTILS_IO_TASK_H

#include <libgs/http/cxx/attributes.h>

namespace libgs::http
{

template <core_concepts::expected_value Value, bool Async = true>
class LIBGS_HTTP_TAPI io_task
{
	LIBGS_DISABLE_COPY(io_task)

public:
	using value_t = Value;
	using expected_t = sys_expected<value_t>;
	using awaitable_t = awaitable<expected_t>;

	explicit io_task(awaitable_t &&task);
	~io_task();

	io_task(io_task &&other) noexcept;
	io_task &operator=(io_task &&other) noexcept;

public:
	template <typename Rep, typename Period>
	auto sync(const duration<Rep,Period> &timeout = std::chrono::milliseconds(0));

	template <typename Clock, typename Duration>
	auto sync(const time_point<Clock,Duration> &timeout);

	template <typename Rep, typename Period>
	auto coro(const duration<Rep,Period> &timeout = std::chrono::milliseconds(0));

	template <typename Clock, typename Duration>
	auto coro(const time_point<Clock,Duration> &timeout);

public:
	static constexpr bool async_enabled_v = Async;
	static consteval bool async_enabled() noexcept;

	auto async(core_concepts::callable<value_t> auto &&callback)
		requires async_enabled_v;

	template <typename Rep, typename Period>
	auto async(const duration<Rep,Period> &timeout,
		core_concepts::callable<value_t> auto &&callback
	) requires async_enabled_v;

	template <typename Clock, typename Duration>
	auto async(const time_point<Clock,Duration> &timeout,
		core_concepts::callable<value_t> auto &&callback
	) requires async_enabled_v;

	template <typename Rep, typename Period>
	auto async(const duration<Rep,Period> &timeout = std::chrono::milliseconds(0))
		requires async_enabled_v;

public:
	template <typename Func>
	static constexpr bool transform_v = requires(expected_t exp, Func func) {
		exp.transform(func);
	};
	template <typename Func>
	auto transform(Func &&func) const requires transform_v<Func>;

	template <typename Func>
	static constexpr bool and_then_v = requires(expected_t exp, Func func) {
		exp.and_then(func);
	};
	template <typename Func>
	auto and_then(Func &&func) const requires and_then_v<Func>;

	template <typename Token>
	static constexpr bool or_else_v = requires(expected_t exp, Token token) {
		exp.or_else(token);
	};
	template <typename Func>
	auto or_else(Func &&func) const requires or_else_v<Func>;

private:
	class impl;
	impl *m_impl = nullptr;
};

} //namespace libgs::http
#include <libgs/http/utils/detail/io_task.h>


#endif //LIBGS_HTTP_UTILS_IO_TASK_H