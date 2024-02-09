
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

#ifndef LIBGS_IO_DETAIL_TIMER_H
#define LIBGS_IO_DETAIL_TIMER_H

namespace libgs
{

template <typename Exec>
timer::timer(const duration &rtime, Exec &exec) :
	base_type(exec, rtime)
{

}

template <typename Exec>
timer::timer(const time_point &atime, Exec &exec) :
	base_type(exec, atime)
{

}

template <typename Exec>
timer::timer(Exec &exec) :
	base_type(exec)
{

}

inline timer::timer(timer &&other) noexcept :
	base_type(std::move(other))
{

}

inline timer &timer::operator=(timer &&other) noexcept
{
	base_type::operator=(std::move(other));
	return *this;
}

template <concept_timer_callback Func>
auto timer::async_wait(const duration &rtime, Func &&func)
{
	expires_after(rtime);
	return base_type::async_wait(std::forward<Func>(func));
}

template <concept_timer_callback Func>
auto timer::async_wait(const time_point &atime, Func &&func)
{
	expires_at(atime);
	return base_type::async_wait(std::forward<Func>(func));
}

inline auto timer::async_wait(const duration &rtime, asio::error_code &error)
{
	expires_after(rtime);
	return base_type::async_wait(use_awaitable_e[error]);
}

inline auto timer::async_wait(const time_point &atime, asio::error_code &error)
{
	expires_at(atime);
	return base_type::async_wait(use_awaitable_e[error]);
}

inline awaitable<asio::error_code> timer::async_wait(const duration &rtime)
{
	asio::error_code error;
	co_await async_wait(rtime, error);
	co_return error;
}

inline awaitable<asio::error_code> timer::async_wait(const time_point &atime)
{
	asio::error_code error;
	co_await async_wait(atime, error);
	co_return error;
}

inline auto timer::async_wait(asio::error_code &error)
{
	return base_type::async_wait(use_awaitable_e[error]);
}

inline awaitable<asio::error_code> timer::async_wait()
{
	asio::error_code error;
	co_await base_type::async_wait(use_awaitable_e[error]);
	co_return error;
}

} //namespace libgs


#endif //LIBGS_IO_DETAIL_TIMER_H
