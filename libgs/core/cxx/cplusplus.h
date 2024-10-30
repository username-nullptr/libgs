
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

#define LIBGS_ARG_T(t)  t
#define LIBGS_ARG_N(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31,a32,a33,a34,a35,a36,a37,a38,a39,a40,a41,a42,a43,a44,a45,a46,a47,a48,a49,a50,a51,a52,a53,a54,a55,a56,a57,a58,a59,a60,a61,a62,a63,N,...)  N
#define LIBGS_ARG_N_HELPER(...)  LIBGS_ARG_T(LIBGS_ARG_N(__VA_ARGS__))

#define LIBGS_ARG_COUNT(...)  LIBGS_ARG_N_HELPER(,##__VA_ARGS__,63,62,61,60,59,57,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define LIBGS_MACRO_N(prefix,...)  LIBGS_CAT(prefix, LIBGS_ARG_COUNT(__VA_ARGS__))

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
