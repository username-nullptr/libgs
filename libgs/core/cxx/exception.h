
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_EXCEPTION_H
#define LIBGS_CORE_CXX_EXCEPTION_H

#include <libgs/core/cxx/utilities.h>
#include <exception>
#include <format>

namespace libgs
{

class runtime_error : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
    ~runtime_error() noexcept override = default;

	template <typename Arg0, typename...Args>
	runtime_error(std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);
};

class system_error : public std::system_error
{
public:
	using std::system_error::system_error;
    ~system_error() noexcept override = default;

	template <typename Arg0, typename...Args>
    system_error(std::error_code ec, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

	template <typename Arg0, typename...Args>
    system_error(int v, const std::error_category& ecat, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);
};

} //namespace libgs
#include <libgs/core/cxx/detail/exception.h>


#endif //LIBGS_CORE_CXX_EXCEPTION_H
