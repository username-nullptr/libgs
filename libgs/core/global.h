
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

#ifndef LIBGS_CORE_GLOBAL_H
#define LIBGS_CORE_GLOBAL_H

#include <libgs/core/cxx/cplusplus.h>
#include <libgs/core/cxx/formatter.h>
#include <libgs/core/cxx/exception.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_API std::string version_string();

template<typename Rep, typename Period>
LIBGS_CORE_TAPI void sleep_for(const std::chrono::duration<Rep,Period> &rtime);

template<typename Clock, typename Duration>
LIBGS_CORE_TAPI void sleep_until(const std::chrono::time_point<Clock,Duration> &atime);

LIBGS_CORE_API std::thread::id this_thread_id();

[[noreturn]] LIBGS_CORE_API void forced_termination();

using error_code = asio::error_code;

namespace errc = asio::error;

} //namespace libgs
#include <libgs/core/detail/global.h>


#endif //LIBGS_CORE_GLOBAL_H
