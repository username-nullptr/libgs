
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

#ifndef LIBGS_IO_TIMER_H
#define LIBGS_IO_TIMER_H

#include <libgs/core/coroutine.h>

namespace libgs
{

template <typename Func>
concept concept_timer_callback = requires(Func &&func) {
	func(asio::error_code());
};

class timer : public asio::steady_timer
{
	LIBGS_DISABLE_COPY(timer)
	using base_type = asio::steady_timer;
	using awaitable = libgs::awaitable<asio::error_code>;

public:
	template <typename Exec = asio::io_context>
	explicit timer(const duration &rtime, Exec &exec = io_context());

	template <typename Exec = asio::io_context>
	explicit timer(const time_point &atime, Exec &exec = io_context());

	template <typename Exec = asio::io_context>
	explicit timer(Exec &exec = io_context());

public:
	timer(timer &&other) noexcept;
	timer &operator=(timer &&other) noexcept;

public:
	template <concept_timer_callback Func>
	auto async_wait(const duration &rtime, Func &&func);

	template <concept_timer_callback Func>
	auto async_wait(const time_point &atime, Func &&func);

public:
	auto async_wait(const duration &rtime, asio::error_code &error);
	auto async_wait(const time_point &atime, asio::error_code &error);

public:
	awaitable async_wait(const duration &rtime);
	awaitable async_wait(const time_point &atime);

public:
	auto async_wait(asio::error_code &error);
	awaitable async_wait();

public:
	using base_type::async_wait;
};

} //namespace libgs
#include <libgs/io/detail/timer.h>


#endif //LIBGS_IO_TIMER_H

