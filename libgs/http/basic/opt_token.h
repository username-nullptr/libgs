
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

#ifndef LIBGS_HTTP_BASIC_OPT_TOKEN_H
#define LIBGS_HTTP_BASIC_OPT_TOKEN_H

#include <libgs/http/basic/types.h>

namespace libgs { namespace http
{

using begin_t = size_t;
using total_t = size_t;
using end_t   = size_t;

template <typename...Args>
using callback_t = std::function<void(Args...)>;

struct range
{
	size_t begin = 0;
	size_t end = 0;
};
using ranges = std::vector<range>;

struct file_range
{
	size_t begin = 0;
	size_t total = 0;

	constexpr file_range();
	file_range(size_t total);
	file_range(size_t begin, size_t total);
};

} //namespace http

#if 0
namespace io
{

using begin_t = http::begin_t;
using total_t = http::total_t;
using end_t   = http::end_t;

template <typename T>
struct opt_token<begin_t,total_t,T> : opt_token<T>
{
	using opt_token<T>::opt_token;
	size_t begin = 0;
	size_t total = 0;

	template <typename...Args>
	opt_token(size_t total, Args&&...args) requires io::concept_opt_token<T,Args...>;

	template <typename...Args>
	opt_token(size_t begin, size_t total, Args&&...args) requires io::concept_opt_token<T,Args...>;
};

template <typename T>
struct opt_token<http::redirect,T> : opt_token<T>
{
	using opt_token<T>::opt_token;
	http::redirect type = http::redirect::moved_permanently;

	template <typename...Args>
	opt_token(http::redirect type, Args&&...args) requires io::concept_opt_token<T,Args...>;
};

template <typename T>
struct opt_token<http::ranges,T> : opt_token<T>
{
	using opt_token<T>::opt_token;
	http::ranges ranges;

	template <typename...Args>
	opt_token(size_t begin, size_t end, Args&&...args) requires io::concept_opt_token<T,Args...>;

	template <typename...Args>
	opt_token(http::ranges ranges, Args&&...args) requires io::concept_opt_token<T,Args...>;
};

template <concept_char_type CharT, typename T>
struct opt_token<http::basic_headers<CharT>,T> : opt_token<T>
{
	using opt_token<T>::opt_token;
	http::basic_headers<CharT> headers;

	template <typename...Args>
	opt_token(http::basic_headers<CharT> headers, Args&&...args) requires io::concept_opt_token<T,Args...>;
};

}} //namespace libgs::io
#else
} //namespace libgs::http
#endif
#include <libgs/http/basic/detail/opt_token.h>


#endif //LIBGS_HTTP_BASIC_OPT_TOKEN_H
