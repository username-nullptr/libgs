
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

namespace libgs::http
{

template <core_concepts::expected_value Value, bool Async>
class LIBGS_HTTP_TAPI io_task<Value,Async>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(awaitable_t &&task) :
		m_task(std::make_shared<awaitable_t>(std::move(task))) {}

	~impl()
	{
		if( m_task )
			sync();
	}

public:
	template <typename Rep, typename Period>
	auto sync(const duration<Rep,Period> &timeout) noexcept
	{

	}

	template <typename Clock, typename Duration>
	auto sync(const time_point<Clock,Duration> &timeout) noexcept
	{

	}

	auto sync() noexcept
	{

	}

public:
	template <typename Rep, typename Period>
	auto coro(const duration<Rep,Period> &timeout = std::chrono::milliseconds(0))
	{

	}

	template <typename Clock, typename Duration>
	auto coro(const time_point<Clock,Duration> &timeout)
	{

	}

	auto coro()
	{

	}

public:
	std::shared_ptr<awaitable_t> m_task;
};

template <core_concepts::expected_value Value, bool Async>
io_task<Value,Async>::io_task(awaitable_t &&task) :
	m_impl(new impl(std::move(task)))
{

}

template <core_concepts::expected_value Value, bool Async>
io_task<Value,Async>::~io_task()
{
	delete m_impl;
}

template <core_concepts::expected_value Value, bool Async>
template <typename Rep, typename Period>
auto io_task<Value,Async>::coro(const duration<Rep,Period> &timeout)
{
	return m_impl->coro(timeout);
}

template <core_concepts::expected_value Value, bool Async>
template <typename Clock, typename Duration>
auto io_task<Value,Async>::coro(const time_point<Clock,Duration> &timeout)
{
	return m_impl->coro(timeout);
}


} //namespace libgs::http


#endif //LIBGS_HTTP_UTILS_DETAIL_IO_TASK_H