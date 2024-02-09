
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

#ifndef LIBGS_CORE_CXX_UTILITIES_H
#define LIBGS_CORE_CXX_UTILITIES_H

#include <libgs/core/cxx/remove_repeat.h>
#include <libgs/core/cxx/type_traits.hpp>
#include <rttr/variant.h>

#ifdef __GNUC__
# include <cxxabi.h>
# define LIBGS_ABI_CXA_DEMANGLE(name)  abi::__cxa_demangle(name, nullptr, nullptr, nullptr)
#else //_MSVC
# define LIBGS_ABI_CXA_DEMANGLE(name)  name
#endif //compiler

#define LIBGS_UNUSED(x)  (void)(x)

#if defined(_WIN64) || defined(__x86_64__) || defined(__arm64__) || defined(__aarch64__)
# define LIBGS_OS_64BIT
#else
# define LIBGS_OS_32BIT
#endif // 32bit & 64bit

#define LIBGS_DISABLE_COPY(_class) \
	explicit _class(const _class&) = delete; \
	void operator=(const _class&) = delete;

#define LIBGS_DISABLE_MOVE(_class) \
	explicit _class(_class&&) = delete; \
	void operator=(_class&&) = delete;

#define LIBGS_DISABLE_COPY_MOVE(_class) \
	LIBGS_DISABLE_COPY(_class) LIBGS_DISABLE_MOVE(_class)

#define __LIBGS_WCHAR(s) L##s
#define LIBGS_WCHAR(s) __LIBGS_WCHAR(s)

namespace libgs
{

template <typename T>
constexpr T &remove_const_v(const T &v);

template <typename T>
const char *type_name(T &&t);

template <typename T>
const char *type_name();

template <typename T>
auto type_id();

auto type_id(const std::string &name);

std::string wcstombs(const std::wstring &str);
char wcstombs(wchar_t c);

std::wstring mbstowcs(const std::string &str);
wchar_t mbstowcs(char c);

} //namespace libgs
#include <libgs/core/cxx/detail/utilities.h>


#endif //LIBGS_CORE_CXX_UTILITIES_H
