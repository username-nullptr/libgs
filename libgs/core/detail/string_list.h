
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

#ifndef LIBGS_CORE_DETAIL_STRING_LIST_H
#define LIBGS_CORE_DETAIL_STRING_LIST_H

namespace libgs
{

template <concept_char_type CharT>
std::vector<std::basic_string<CharT>> basic_string_list<CharT>::to_vector() const
{
	std::vector<str_type> result;
	result.reserve(this->size());

	for(size_t i=0; i<this->size(); i++)
		result[i] = base_type::operator[](i);
	return result;
}

template <concept_char_type CharT>
std::vector<const CharT*> basic_string_list<CharT>::c_str_vector() const
{
	std::vector<const char*> result;
	result.reserve(this->size());

	for(size_t i=0; i<this->size(); i++)
		result[i] = base_type::operator[](i).c_str();
	return result;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_string_list<CharT>::join(const str_type &splits)
{
	return str_list_join(*this, splits);
}

template <concept_char_type CharT>
basic_string_list<CharT> basic_string_list<CharT>::from_string
(str_view_type str, str_view_type splits, bool ignore_empty)
{
	basic_string_list<CharT> result;
	if( str.empty() )
		return result;

	auto strs = str_type(str.data(), str.size()) + str_type(splits.data(), splits.size());
	auto pos = strs.find(splits);
	auto step = splits.size();

	while( pos != std::basic_string<CharT>::npos )
	{
		auto tmp = strs.substr(0, pos);
		if( not str_trimmed(tmp).empty() or not ignore_empty )
			result.emplace_back(std::move(tmp));

		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
	return result;
}

template <concept_char_type CharT>
basic_string_list<CharT> basic_string_list<CharT>::from_string
(str_view_type str, CharT splits, bool ignore_empty)
{
	return from_string(str, str_type(1,splits), ignore_empty);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_STRING_LIST_H
