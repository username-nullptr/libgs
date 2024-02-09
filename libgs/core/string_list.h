
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

#ifndef LIBGS_CORE_STRING_LIST_H
#define LIBGS_CORE_STRING_LIST_H

#include <libgs/core/global.h>
#include <string>
#include <vector>
#include <deque>

namespace libgs
{

template <concept_char_type CharT>
using basic_string_deque = std::deque<std::basic_string<CharT>>;

using string_deque = basic_string_deque<char>;

using wstring_deque = basic_string_deque<wchar_t>;

template <concept_char_type CharT>
class basic_string_list : public basic_string_deque<CharT>
{
	template <typename T>
	struct default_splits_argument {};

	template <>
	struct default_splits_argument<char>
	{
		static constexpr const char *s = " ";
		static constexpr char c = ' ';
	};

	template <>
	struct default_splits_argument<wchar_t>
	{
		static constexpr const wchar_t *s = L" ";
		static constexpr wchar_t c = L' ';
	};

	static constexpr const CharT *default_splits_argument_s = default_splits_argument<CharT>::s;
	static constexpr const CharT *default_splits_argument_c = default_splits_argument<CharT>::c;

public:
	using str_type = std::basic_string<CharT>;
	using base_type = basic_string_deque<CharT>;
	using base_type::base_type;

public:
	[[nodiscard]] std::vector<str_type> to_vector() const;
	[[nodiscard]] std::vector<const CharT*> c_str_vector() const;

public:
	[[nodiscard]] str_type join(const str_type &splits = default_splits_argument_s);

	[[nodiscard]] static basic_string_list<CharT>
		from_string(const str_type &str, const str_type &splits = default_splits_argument_s,
					bool ignore_empty = true);

	[[nodiscard]] static basic_string_list<CharT>
		from_string(const str_type &str, CharT splits = default_splits_argument_c,
					bool ignore_empty = true);
};

using string_list = basic_string_list<char>;

using wstring_list = basic_string_list<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/string_list.h>


#endif //LIBGS_CORE_STRING_LIST_H
