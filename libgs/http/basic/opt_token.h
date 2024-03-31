
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

#include <libgs/http/global.h>
#include <libgs/io/types/opt_token.h>

namespace libgs::http
{

template <typename T>
using read_token = io::read_token<T>;

template <typename...Args>
using read_cb_token = io::read_cb_token<Args...>;

namespace detail
{

template <typename T>
struct save_file_token : io::opt_token<T>
{
	using io::opt_token<T>::opt_token;
	size_t total_size = 0;

	template <typename...Args>
	save_file_token(size_t total_size, Args&&...args) requires io::concept_opt_token<T,Args...> :
		io::opt_token<T>(std::forward<Args>(args)...), total_size(total_size) {}
};

} //namespace detail

template <typename T>
struct save_file_token : detail::save_file_token<T>
{
	using detail::save_file_token<T>::save_file_token;
	size_t begin = 0;

	template <typename...Args>
	save_file_token(size_t begin, Args&&...args) :
			detail::save_file_token<T>(std::forward<Args>(args)...), begin(begin) {}
};

template <typename...Args>
using save_file_cb_token = save_file_token<io::callback_t<Args...>>;

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_OPT_TOKEN_H
