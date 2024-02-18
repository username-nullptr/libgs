
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

#ifndef LIBGS_IO_DETAIL_DEVICE_BASE_H
#define LIBGS_IO_DETAIL_DEVICE_BASE_H

namespace libgs::io
{

template <concept_execution Exec>
device_base<Exec>::device_base(const executor_type &exec) :
	m_exec(exec),
	m_valid(new bool(true))
{

}

template <concept_execution Exec>
device_base<Exec>::~device_base() 
{
	*m_valid = false;
}

template <concept_execution Exec>
device_base<Exec>::executor_type &device_base<Exec>::executor() const
{
	return m_exec;
}

template <typename Token>
template <typename Rep, typename Period>
opt_token<Token>::opt_token(const duration<Rep, Period> &rtime) :
	rtime(std::chrono::duration_cast<std::chrono::nanoseconds>(rtime))
{

}

template <typename Token>
template<typename Clock, typename Duration>
opt_token<Token>::opt_token(const time_point<Clock, Duration> &atime)
{
	namespace dt = std::chrono;
	auto _rtime = atime - dt::time_point_cast<time_point<Clock, Duration>>(dt::system_clock::now());
	rtime = dt::duration_cast<dt::nanoseconds>(_rtime);
}

template <typename...Args>
template <typename Func>
opt_token<callback_t<Args...>>::opt_token(Func &&callback)
	requires concept_void_callable<Func,Args...> :
	callback(std::forward<Func>(callback))
{

}

template <typename...Args>
template <typename Rep, typename Period, typename Func>
opt_token<callback_t<Args...>>::opt_token(const duration<Rep, Period> &rtime, Func &&callback)
	requires concept_void_callable<Func,Args...> :
	base_type(rtime), callback(std::forward<Func>(callback))
{
	
}

template <typename...Args>
template<typename Clock, typename Duration, typename Func>
opt_token<callback_t<Args...>>::opt_token(const time_point<Clock, Duration> &atime, Func &&callback) 
	requires concept_void_callable<Func,Args...> :
	base_type(atime), callback(std::forward<Func>(callback))
{

}

inline opt_token<use_awaitable_t &>::opt_token(use_awaitable_t &ua) :
	ua(ua)
{

}

template <typename Rep, typename Period>
opt_token<use_awaitable_t&>::opt_token(const duration<Rep, Period> &rtime, use_awaitable_t &ua) :
	base_type(rtime), ua(ua)
{

}

template<typename Clock, typename Duration>
opt_token<use_awaitable_t&>::opt_token(const time_point<Clock, Duration> &atime, use_awaitable_t &ua) :
	base_type(atime), ua(ua)
{

}

inline opt_token<ua_redirect_error_t>::opt_token(ua_redirect_error_t uare) :
	uare(std::move(uare))
{

}

template <typename Rep, typename Period>
opt_token<ua_redirect_error_t>::opt_token(const duration<Rep, Period> &rtime, ua_redirect_error_t uare) :
	base_type(rtime), uare(std::move(uare))
{

}

template<typename Clock, typename Duration>
opt_token<ua_redirect_error_t>::opt_token(const time_point<Clock, Duration> &atime, ua_redirect_error_t uare) :
	base_type(atime), uare(std::move(uare))
{

}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_DEVICE_BASE_H
