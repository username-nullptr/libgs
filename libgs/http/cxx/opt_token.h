
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

#ifndef LIBGS_HTTP_CXX_OPT_TOKEN_H
#define LIBGS_HTTP_CXX_OPT_TOKEN_H

#include <libgs/http/global.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI basic_path_opt_token
{
	using char_t = CharT;
	using string_view_t = std::basic_string_view<char_t>;
	std::list<string_view_t> paths;

	template <core_concepts::basic_string_type<CharT> Str>
	basic_path_opt_token(Str &&path);

	template <core_concepts::basic_string_type<CharT> Str>
	basic_path_opt_token(std::list<Str> &&paths);

	template <core_concepts::basic_string_type<CharT> Str>
	basic_path_opt_token(std::initializer_list<Str> paths);

	template <core_concepts::basic_string_type<CharT>...Str>
	basic_path_opt_token(Str&&...paths);
};

using path_opt_token  = basic_path_opt_token<char>;
using wpath_opt_token = basic_path_opt_token<wchar_t>;

} //namespace libgs::http::operators
#include <libgs/http/cxx/detail/opt_token.h>


#endif //LIBGS_HTTP_CXX_OPT_TOKEN_H