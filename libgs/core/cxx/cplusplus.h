
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

#ifndef LIBGS_CORE_CXX_CPLUSPLUS_H
#define LIBGS_CORE_CXX_CPLUSPLUS_H

#ifdef _MSC_VER // _MSVC
# define LIBGS_CPLUSPLUS  _MSVC_LANG
#else
# define LIBGS_CPLUSPLUS  __cplusplus
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__arm64__) || defined(__aarch64__)
# define LIBGS_OS_64BIT
#else
# define LIBGS_OS_32BIT
#endif // 32bit & 64bit

#define LIBGS_UNUSED(x)  (void)(x)

#define LIBGS_CAT_IMPL(a,b)  a##b
#define LIBGS_CAT(a,b)  LIBGS_CAT_IMPL(a,b)

#define LIBGS_AUTO_XX_NAME(_prefix)  LIBGS_CAT(_prefix,__LINE__)

#define LIBGS_DISABLE_COPY(_class) \
	explicit _class(const _class&) = delete; \
	void operator=(const _class&) = delete; \
	void operator=(const _class&) volatile = delete;

#define LIBGS_DISABLE_MOVE(_class) \
	explicit _class(_class&&) = delete; \
	void operator=(_class&&) = delete; \
	void operator=(_class&&) volatile = delete;

#define LIBGS_DISABLE_COPY_MOVE(_class) \
	LIBGS_DISABLE_COPY(_class) LIBGS_DISABLE_MOVE(_class)

#ifndef ASIO_HAS_CHRONO
# define ASIO_HAS_CHRONO
#endif //ASIO_HAS_CHRONO

#ifndef SPDLOG_USE_STD_FORMAT
# define SPDLOG_USE_STD_FORMAT
#endif //SPDLOG_USE_STD_FORMAT

#ifdef _MSC_VER // _MSVC
# pragma warning(disable: 4819)
#endif // _MSVC


#endif //LIBGS_CORE_CXX_CPLUSPLUS_H
