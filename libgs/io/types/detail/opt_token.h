
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

#ifndef LIBGS_IO_TYPES_DETAIL_OPT_TOKEN_H
#define LIBGS_IO_TYPES_DETAIL_OPT_TOKEN_H

namespace libgs::io
{

template <typename Rep, typename Period>
opt_token<void>::opt_token(const duration<Rep,Period> &rtime) :
	rtime(std::chrono::duration_cast<std::chrono::nanoseconds>(rtime))
{

}

template<typename Clock, typename Duration>
opt_token<void>::opt_token(const time_point<Clock,Duration> &atime)
{
	namespace dt = std::chrono;
	auto _rtime = atime - dt::time_point_cast<time_point<Clock,Duration>>(dt::system_clock::now());
	rtime = dt::duration_cast<dt::nanoseconds>(_rtime);
}

template <typename...Args>
template <typename Func>
opt_token<callback_t<Args...>>::opt_token(Func &&callback) requires concept_void_callable<Func,Args...> :
	callback(std::forward<Func>(callback))
{

}

template <typename...Args>
template <typename Time, typename Func>
opt_token<callback_t<Args...>>::opt_token(const Time &time, Func &&callback) requires 
	concept_opt_token<void,Time> and concept_void_callable<Func,Args...> :
	opt_token<void>(time), callback(std::forward<Func>(callback))
{

}

inline opt_token<error_code&>::opt_token(error_code &error) :
	error(&error)
{

}

template <typename Time>
opt_token<error_code&>::opt_token(const Time &time, error_code &error) requires concept_opt_token<void,Time> :
	opt_token<void>(time), error(&error)
{

}

template <typename T>
template <typename...Args>
opt_token<read_condition,T>::opt_token(read_condition rc, Args&&...args) requires concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), rc(std::move(rc))
{

}

} //namespace libgs::io


#endif //LIBGS_IO_TYPES_DETAIL_OPT_TOKEN_H
