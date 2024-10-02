
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

template <concepts::char_type CharT>
std::vector<std::basic_string<CharT>> basic_string_list<CharT>::to_vector() const
{
	std::vector<string_t> result;
	result.reserve(this->size());

	for(size_t i=0; i<this->size(); i++)
		result[i] = base_t::operator[](i);
	return result;
}

template <concepts::char_type CharT>
std::vector<const CharT*> basic_string_list<CharT>::c_str_vector() const
{
	std::vector<const char*> result;
	result.reserve(this->size());

	for(size_t i=0; i<this->size(); i++)
		result[i] = base_t::operator[](i).c_str();
	return result;
}

template <concepts::char_type CharT>
std::basic_string<CharT> basic_string_list<CharT>::join(const string_t &splits)
{
	string_t result;
	for(auto &str : *this)
		result += str + splits;
	result.erase(result.size() - splits.size(), splits.size());
	return result;
}

template <concepts::char_type CharT>
std::basic_string<CharT> basic_string_list<CharT>::join(size_t index, size_t length, const string_t &splits)
{
	string_t result;
	auto end = index + length;

	if( end > this->size() )
	{
		end = this->size();
		if( end <= index )
			return result;
	}
	while( index < end )
		result += (*this)[index++] + splits;
	result.erase(result.size() - splits.size(), splits.size());
	return result;
}

template <concepts::char_type CharT>
std::basic_string<CharT> basic_string_list<CharT>::join(size_t index, const string_t &splits)
{
	return join(index, this->size(), splits);
}

template <concepts::char_type CharT>
std::basic_string<CharT> basic_string_list<CharT>::join
(detail::concepts::string_list_iterator<CharT> auto begin,
 detail::concepts::string_list_iterator<CharT> auto end,
 const string_t &splits)
{
	string_t result;
	for(auto it=begin; it!=end; ++it)
		result += *it + splits;
	result.erase(result.size() - splits.size(), splits.size());
	return result;
}

template <concepts::char_type CharT>
basic_string_list<CharT> basic_string_list<CharT>::from_string
(string_view_t str, string_view_t splits, bool ignore_empty)
{
	basic_string_list<CharT> result;
	if( str.empty() )
		return result;

	auto strs = string_t(str.data(), str.size()) + string_t(splits.data(), splits.size());
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

template <concepts::char_type CharT>
basic_string_list<CharT> basic_string_list<CharT>::from_string
(string_view_t str, CharT splits, bool ignore_empty)
{
	return from_string(str, string_t(1,splits), ignore_empty);
}

} //namespace libgs

namespace std
{

template <libgs::concepts::char_type CharT>
class formatter<libgs::basic_string_list<CharT>, CharT>
{
public:
	auto format(const libgs::basic_string_list<CharT> &slist, auto &context) const
	{
		if( slist.empty() )
			return m_formatter.format("[]", context);

		if constexpr( libgs::is_char_v<CharT> )
		{
			std::string buf = "[";
			for(auto &str : slist)
				buf += "'" + str + "', ";
			buf.erase(buf.size() - 2, 2);
			buf += "]";
			return m_formatter.format(buf, context);
		}
		else
		{
			std::wstring buf = L"[";
			for(auto &str : slist)
				buf += L"'" + str + L"', ";
			buf.erase(buf.size() - 2, 2);
			buf += L"]";
			return m_formatter.format(buf, context);
		}
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};

} //namespace std


#endif //LIBGS_CORE_DETAIL_STRING_LIST_H
