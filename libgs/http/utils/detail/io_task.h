
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

#ifndef LIBGS_HTTP_UTILS_DETAIL_IO_TASK_H
#define LIBGS_HTTP_UTILS_DETAIL_IO_TASK_H

#include <libgs/coro/utils.h>

namespace libgs::http
{

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
class LIBGS_HTTP_TAPI basic_io_task<Exec,Value,Async>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(const auto &exec, awaitable_t &&coro_task, sync_func_t &&sync_task) :
		m_coro_task(std::move(coro_task)),
		m_sync_task(std::move(sync_task)),
		m_exec(exec) {}

	~impl()
	{
		if( m_sync_task )
			sync();
	}

public:
	impl *check_task(std::string_view msg)
	{
		if( not m_sync_task )
			throw logic_error("{}: The task has been completed.", msg);
		return this;
	}

	expected_t sync() noexcept
	{
		auto res = m_sync_task();
		m_sync_task = {};
		return res;
	}

public:
	template <typename Rep, typename Period>
	[[nodiscard]] awaitable_t coro(const duration<Rep,Period> &timeout) {
		return coro(std::chrono::system_clock::now() + timeout);
	}

	template <typename Clock, typename Duration>
	[[nodiscard]] awaitable_t coro(const time_point<Clock,Duration> &timeout)
	{
		expected_t expected;
		auto exec = co_await coro::goto_exec(m_exec);
		try {
			auto var = co_await (
				std::move(*m_coro_task) or coro::sleep_until(timeout)
			);
			if( var.index() == 0 )
				expected = std::move(std::get<0>(var));
			else
			{
				expected.despair (
					make_error_code(errc::timed_out)
				);
			}
		}
		catch(const std::system_error &ex) {
			expected.despair(ex.code());
		}
		catch(...)
		{
			expected.despair (
				make_error_code(std::errc::io_error)
			);
		}
		m_sync_task = {};
		co_await coro::goto_exec(exec);
		co_return expected;
	}

	[[nodiscard]] awaitable_t coro()
	{
		expected_t expected;
		auto exec = co_await coro::goto_exec(m_exec);
		try {
			expected = co_await std::move(*m_coro_task);
		}
		catch(const std::system_error &ex) {
			expected.despair(ex.code());
		}
		catch(...)
		{
			expected.despair (
				make_error_code(std::errc::io_error)
			);
		}
		m_sync_task = {};
		co_await coro::goto_exec(exec);
		co_return expected;
	}

public:
	template <typename Rep, typename Period>
	void async(const duration<Rep,Period> &timeout, core_concepts::callable<expected_t> auto &&callback)
	{
		using callback_t = decltype(callback);
		dispatch(m_exec, coro(timeout), std::forward<callback_t>(callback));
	}

	template <typename Clock, typename Duration>
	void async(const time_point<Clock,Duration> &timeout, core_concepts::callable<expected_t> auto &&callback)
	{
		using callback_t = decltype(callback);
		dispatch(m_exec, coro(timeout), std::forward<callback_t>(callback));
	}

	void async(core_concepts::callable<expected_t> auto &&callback)
	{
		using callback_t = decltype(callback);
		dispatch(m_exec, coro(), std::forward<callback_t>(callback));
	}

	template <typename Rep, typename Period>
	[[nodiscard]] future_t async(const duration<Rep,Period> &timeout)
	{
		if( timeout.count() == 0 )
			return dispatch(m_exec, coro(), use_future);
		return dispatch(m_exec, coro(timeout), use_future);
	}

	template <typename Clock, typename Duration>
	[[nodiscard]] future_t async(const time_point<Clock,Duration> &timeout) {
		return dispatch(m_exec, coro(timeout), use_future);
	}

public:
	template <typename Rep, typename Period>
	void detach(const duration<Rep,Period> &timeout) noexcept
	{
		if( timeout.count() == 0 )
			dispatch(m_exec, coro(), detached);
		dispatch(m_exec, coro(timeout), detached);
	}

	template <typename Clock, typename Duration>
	void detach(const time_point<Clock,Duration> &timeout) noexcept {
		dispatch(m_exec, coro(timeout), detached);
	}

public:
	awaitable_t m_coro_task;
	sync_func_t m_sync_task;
	executor_t m_exec;
};

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
basic_io_task<Exec,Value,Async>::basic_io_task
(core_concepts::sched auto &&exec, awaitable_t &&coro_task, sync_func_t &&sync_task) :
	m_impl(new impl (
		get_executor_helper(std::forward<decltype(exec)>(exec)),
		std::move(coro_task), std::move(sync_task)
	))
{

}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
basic_io_task<Exec,Value,Async>::basic_io_task(awaitable_t &&coro_task, sync_func_t &&sync_task) :
	m_impl(new impl(get_executor(), std::move(coro_task), std::move(sync_task)))
{

}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
basic_io_task<Exec,Value,Async>::~basic_io_task()
{
	delete m_impl;
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
basic_io_task<Exec,Value,Async>::expected_t basic_io_task<Exec,Value,Async>::sync()
{
	return m_impl->check_task("libgs::http::io_task::sync")->sync();
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Rep, typename Period>
basic_io_task<Exec,Value,Async>::awaitable_t
basic_io_task<Exec,Value,Async>::coro(const duration<Rep,Period> &timeout)
{
	m_impl->check_task("libgs::http::io_task::coro");
	return timeout.count() == 0 ? m_impl->coro() : m_impl->coro(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Clock, typename Duration>
basic_io_task<Exec,Value,Async>::awaitable_t
basic_io_task<Exec,Value,Async>::coro(const time_point<Clock,Duration> &timeout)
{
	return m_impl->check_task("libgs::http::io_task::coro")->coro(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
consteval bool basic_io_task<Exec,Value,Async>::async_enabled() noexcept
{
	return async_enabled_v;
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
void basic_io_task<Exec,Value,Async>::async
(core_concepts::callable<expected_t> auto &&callback)
	requires async_enabled_v
{
	m_impl->async(std::forward<decltype(callback)>(callback));
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Rep, typename Period>
void basic_io_task<Exec,Value,Async>::async
(const duration<Rep,Period> &timeout, core_concepts::callable<expected_t> auto &&callback)
	requires async_enabled_v
{
	m_impl->async(timeout, std::forward<decltype(callback)>(callback));
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Clock, typename Duration>
void basic_io_task<Exec,Value,Async>::async
(const time_point<Clock,Duration> &timeout, core_concepts::callable<expected_t> auto &&callback)
	requires async_enabled_v
{
	m_impl->async(timeout, std::forward<decltype(callback)>(callback));
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Rep, typename Period>
basic_io_task<Exec,Value,Async>::future_t
basic_io_task<Exec,Value,Async>::async(const duration<Rep,Period> &timeout)
	requires async_enabled_v
{
	return m_impl->async(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Clock, typename Duration>
basic_io_task<Exec,Value,Async>::future_t
basic_io_task<Exec,Value,Async>::async(const time_point<Clock,Duration> &timeout)
	requires async_enabled_v
{
	return m_impl->async(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Rep, typename Period>
void basic_io_task<Exec,Value,Async>::detach(const duration<Rep,Period> &timeout)
	noexcept requires async_enabled_v
{
	m_impl->detach(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Clock, typename Duration>
void basic_io_task<Exec,Value,Async>::detach(const time_point<Clock,Duration> &timeout)
	noexcept requires async_enabled_v
{
	m_impl->detach(timeout);
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Func>
auto basic_io_task<Exec,Value,Async>::transform(Func &&func)
	const requires transform_v<Func>
{
	return sync().transform(std::forward<Func>(func));
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Func>
auto basic_io_task<Exec,Value,Async>::and_then(Func &&func)
	const requires and_then_v<Func>
{
	return sync().and_then(std::forward<Func>(func));
}

template <core_concepts::exec Exec, core_concepts::expected_value Value, bool Async>
template <typename Token>
auto basic_io_task<Exec,Value,Async>::or_else(Token &&token)
	const requires or_else_v<Token>
{
	return sync().or_else(std::forward<Token>(token));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_UTILS_DETAIL_IO_TASK_H