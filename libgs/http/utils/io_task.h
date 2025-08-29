
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

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async = true>
class LIBGS_HTTP_TAPI basic_io_task
{
	LIBGS_DISABLE_COPY(basic_io_task)

public:
	using executor_t = Exec;
	using value_t = Value;

	using expected_t = sys_expected<value_t>;
	using unexpected_t = sys_unexpected;

	using sync_func_t = std::function<expected_t()>;
	using awaitable_t = awaitable<expected_t>;
	using future_t = std::future<expected_t>;

	basic_io_task(core_concepts::sched auto &&exec, awaitable_t &&coro_task, sync_func_t &&sync_task);
	basic_io_task(awaitable_t &&coro_task, sync_func_t &&sync_task);
	~basic_io_task();

	basic_io_task(basic_io_task &&other) noexcept;
	basic_io_task &operator=(basic_io_task &&other) noexcept;

public:
	expected_t sync();
	[[nodiscard]] awaitable_t coro();

	template <typename Rep, typename Period>
	[[nodiscard]] awaitable_t coro(const duration<Rep,Period> &timeout);

	template <typename Clock, typename Duration>
	[[nodiscard]] awaitable_t coro(const time_point<Clock,Duration> &timeout);

public:
	static constexpr bool async_enabled_v = Async;
	static consteval bool async_enabled() noexcept;

	void async(core_concepts::callable<expected_t> auto &&callback)
		requires async_enabled_v;

	template <typename Rep, typename Period>
	void async(const duration<Rep,Period> &timeout,
		core_concepts::callable<expected_t> auto &&callback
	) requires async_enabled_v;

	template <typename Clock, typename Duration>
	void async(const time_point<Clock,Duration> &timeout,
		core_concepts::callable<expected_t> auto &&callback
	) requires async_enabled_v;

	future_t async() requires async_enabled_v;

	template <typename Rep, typename Period>
	future_t async(const duration<Rep,Period> &timeout)
		requires async_enabled_v;

	template <typename Clock, typename Duration>
	future_t async(const time_point<Clock,Duration> &timeout)
		requires async_enabled_v;

public:
	void detach() noexcept requires async_enabled_v;

	template <typename Rep, typename Period>
	void detach(const duration<Rep,Period> &timeout = std::chrono::milliseconds(0))
		noexcept requires async_enabled_v;

	template <typename Clock, typename Duration>
	void detach(const time_point<Clock,Duration> &timeout)
		noexcept requires async_enabled_v;

public:
	template <typename Func>
	static constexpr bool transform_v = requires(expected_t exp, Func func) {
		exp.transform(func);
	};
	template <typename Func>
	auto transform(Func &&func) requires transform_v<Func>;

	template <typename Func>
	static constexpr bool and_then_v = requires(expected_t exp, Func func) {
		exp.and_then(func);
	};
	template <typename Func>
	auto and_then(Func &&func) requires and_then_v<Func>;

	template <typename Token>
	static constexpr bool or_else_v = requires(expected_t exp, Token token) {
		exp.or_else(token);
	};
	template <typename Token>
	auto or_else(Token &&token) requires or_else_v<Token>;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <core_concepts::expected_value Value, bool Async = true>
using io_task = basic_io_task<asio::any_io_executor, Value, Async>;

} //namespace libgs::http
#include <libgs/http/utils/detail/io_task.h>


#endif //LIBGS_HTTP_UTILS_IO_TASK_H