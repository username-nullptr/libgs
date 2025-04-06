
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_BASE_H
#define LIBGS_CORE_ALGORITHM_DETAIL_BASE_H

#include "libgs/core/cxx/string_tools.h"

namespace libgs { namespace detail
{

auto file_name(const concepts::weak_string_type auto &file_name)
{
	using str_t = std::remove_cvref_t<decltype(file_name)>;
	using char_t = get_string_char_t<str_t>;

	if constexpr( concepts::character<str_t> )
		return libgs::file_name(std::basic_string_view<char_t>(&file_name,1));
	else
	{
		std::basic_string_view<char_t> view(file_name);
		size_t pos = view.rfind(s_str<char_t,'/'>);

		if( pos == std::basic_string<char_t>::npos )
		{
			pos = view.rfind(s_str<char_t,'\\'>);
			if( pos == std::basic_string<char_t>::npos )
				return std::basic_string<char_t>(view.data(), view.size());
		}
		auto tmp = view.substr(pos + 1);
		return std::basic_string<char_t>(tmp.data(), tmp.size());
	}
}

auto file_path(const concepts::weak_string_type auto &file_name)
{
	using str_t = std::remove_cvref_t<decltype(file_name)>;
	using char_t = get_string_char_t<str_t>;

	if constexpr( concepts::character<str_t> )
		return libgs::file_path(std::basic_string_view<char_t>(&file_name,1));
	else
	{
		std::basic_string_view<char_t> view(file_name);
		size_t pos = view.rfind(s_str<char_t,'/'>);

		if( pos == std::basic_string<char_t>::npos )
			return std::basic_string<char_t>(1, static_cast<char_t>('.'));

		auto tmp = view.substr(0, pos + 1);
		return std::basic_string<char_t>(tmp.data(), tmp.size());
	}
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_BASE_H