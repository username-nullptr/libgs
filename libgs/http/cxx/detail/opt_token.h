
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

#ifndef LIBGS_HTTP_CXX_DETAIL_PATH_OPT_TOKEN_H
#define LIBGS_HTTP_CXX_DETAIL_PATH_OPT_TOKEN_H

namespace libgs::http
{

template <core_concepts::char_type CharT>
template <core_concepts::basic_string_type<CharT> Str>
basic_path_opt_token<CharT>::basic_path_opt_token(Str &&path) :
	paths{std::forward<Str>(path)}
{

}

template <core_concepts::char_type CharT>
template <core_concepts::basic_string_type<CharT> Str>
basic_path_opt_token<CharT>::basic_path_opt_token(std::list<Str> &&paths) :
	paths{std::forward<std::list<Str>>(paths)}
{

}

template <core_concepts::char_type CharT>
template <core_concepts::basic_string_type<CharT> Str>
basic_path_opt_token<CharT>::basic_path_opt_token(std::initializer_list<Str> paths)
{
	for(auto &path : paths)
		this->paths.emplace_back(path);
}

template <core_concepts::char_type CharT>
template <core_concepts::basic_string_type<CharT>...Str>
basic_path_opt_token<CharT>::basic_path_opt_token(Str&&...paths)
{
	(void) std::initializer_list<int> {
		(this->paths.emplace_back(std::forward<Str>(paths)), 0) ...
	};
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_PATH_OPT_TOKEN_H
