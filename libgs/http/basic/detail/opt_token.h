
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_OPT_TOKEN_H
#define LIBGS_HTTP_BASIC_DETAIL_OPT_TOKEN_H

namespace libgs::http
{

inline constexpr file_range::file_range()
{

}

inline file_range::file_range(size_t total) :
	total(total)
{

}

inline file_range::file_range(size_t begin, size_t total) :
	begin(begin), total(total)
{

}

namespace detail
{

template <typename T>
concept concept_has_ec_ = requires(T &&t) {
	t.ec_;
};

template <typename T>
concept concept_has_get = requires(T &&t)
{
	t.get_cancellation_slot();
	t.get();
};

}; //namespace detail

} //namespace libgs::http

#if 0
template <typename T>
template <typename...Args>
opt_token<begin_t,total_t,T>::opt_token(size_t total, Args&&...args) requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), total(total)
{

}

template <typename T>
template <typename...Args>
opt_token<begin_t,total_t,T>::opt_token(size_t begin, size_t total, Args&&...args) requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), begin(begin), total(total)
{

}

template <typename T>
template <typename...Args>
opt_token<http::redirect,T>::opt_token(http::redirect type, Args&&...args) requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), type(type)
{

}

template <typename T>
template <typename...Args>
opt_token<http::ranges,T>::opt_token(size_t begin, size_t end, Args&&...args) requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...)
{
	ranges.emplace_back(http::range{begin, end});
}

template <typename T>
template <typename...Args>
opt_token<http::ranges,T>::opt_token(http::ranges ranges, Args&&...args) requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), ranges(std::move(ranges))
{

}

template <concept_char_type CharT, typename T>
template <typename...Args>
opt_token<http::basic_headers<CharT>,T>::opt_token(http::basic_headers<CharT> headers, Args&&...args)
	requires io::concept_opt_token<T,Args...> :
	opt_token<T>(std::forward<Args>(args)...), headers(std::move(headers))
{

}

} //namespace libgs::io
#endif


#endif //LIBGS_HTTP_BASIC_DETAIL_OPT_TOKEN_H

