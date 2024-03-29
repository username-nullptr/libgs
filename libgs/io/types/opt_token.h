
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

#ifndef LIBGS_IO_TYPES_OPT_TOKEN_H
#define LIBGS_IO_TYPES_OPT_TOKEN_H

#include <libgs/core/coroutine.h>
#include <libgs/io/types/opt_condition.h>

namespace libgs::io
{

template <typename...Args>
using callback_t = std::function<void(Args...)>;

template <typename Token>
struct opt_token;

template <>
struct LIBGS_CORE_VAPI opt_token<void>
{
	std::chrono::nanoseconds rtime {0};

	template <typename Rep, typename Period>
	opt_token(const duration<Rep,Period> &rtime);

	template<typename Clock, typename Duration>
	opt_token(const time_point<Clock,Duration> &atime);

	opt_token() = default;
};

template <typename T, typename...Args>
concept concept_opt_token = requires(Args&&...value) {
	opt_token<T>(std::forward<Args>(value)...);
};

template <typename...Args>
struct LIBGS_CORE_VAPI opt_token<callback_t<Args...>> : opt_token<void>
{
	using callback_t = callback_t<Args...>;
	callback_t callback;

	template <typename Func>
	opt_token(Func &&callback) requires concept_void_callable<Func,Args...>;

	template <typename Time, typename Func>
	opt_token(const Time &time, Func &&callback) requires 
		concept_opt_token<void,Time> and concept_void_callable<Func,Args...>;
};

template <typename...Args>
using opt_cb_token = opt_token<callback_t<Args...>>;

template <>
struct LIBGS_CORE_VAPI opt_token<error_code&> : opt_token<void>
{
	using opt_token<void>::opt_token;
	error_code *error = nullptr;

	opt_token(error_code &error);

	template <typename Time>
	opt_token(const Time &time, error_code &error) requires concept_opt_token<void,Time>;
};

template <typename T>
struct LIBGS_CORE_VAPI read_token : opt_token<T>
{
	using opt_token<T>::opt_token;
	read_condition rc;

	template <typename...Args>
	read_token(read_condition rc, Args&&...args) requires concept_opt_token<T,Args...>;
};

template <typename...Args>
using read_cb_token = read_token<callback_t<Args...>>;

} //namespace libgs::io
#include <libgs/io/types/detail/opt_token.h>


#endif //LIBGS_IO_TYPES_OPT_TOKEN_H
