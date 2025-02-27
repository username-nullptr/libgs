
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

#include <libgs/core/algorithm/base.h>

namespace libgs
{

template <concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::weak_basic_string_type<CharT> Str>
std::basic_string<CharT> basic_string_container<CharT,Container,Args...>::join(const Str &splits)
{
	string_t result;
	auto view = transition_string_view(splits);

	for(auto &str : *this)
		result += str + string_t(view.data(), view.size());

	result.erase(result.size() - view.size(), view.size());
	return result;
}

template <concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::weak_basic_string_type<CharT> Str>
std::basic_string<CharT> basic_string_container<CharT,Container,Args...>::join
(size_t index, size_t length, const Str &splits)
{
	string_t result;
	auto view = transition_string_view(splits);

	auto end = index + length;
	if( end > this->size() )
	{
		end = this->size();
		if( end <= index )
			return result;
	}
	while( index < end )
		result += (*this)[index++] + string_t(view.data(), view.size());
	result.erase(result.size() - view.size(), view.size());
	return result;
}

template <concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::weak_basic_string_type<CharT> Str>
std::basic_string<CharT> basic_string_container<CharT,Container,Args...>::join
(size_t index, const Str &splits)
{
	return join(index, this->size(), splits);
}

template <concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::string_list_iterator<CharT,Container,Args...> Iter,
		  concepts::weak_basic_string_type<CharT> Str>
std::basic_string<CharT> basic_string_container<CharT,Container,Args...>::join
(Iter begin, Iter end, const Str &splits)
{
	string_t result;
	auto view = transition_string_view(splits);

	for(auto it=begin; it!=end; ++it)
		result += *it + string_t(view.data(), view.size());

	result.erase(result.size() - view.size(), view.size());
	return result;
}

template <concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::weak_basic_string_type<CharT> Str>
basic_string_container<CharT,Container,Args...>
basic_string_container<CharT,Container,Args...>::from_string
(concepts::basic_string_type<char_t> auto &&str, const Str &splits, bool ignore_empty)
{
	basic_string_container result;
	auto view = transition_string_view(splits);

	if( view.empty() )
		return result;

	string_t strs(std::forward<Str>(str));
	strs += view;

	auto pos = strs.find(view);
	auto step = view.size();

	while( pos != std::basic_string<CharT>::npos )
	{
		auto tmp = strs.substr(0, pos);
		if( not str_trimmed(tmp).empty() or not ignore_empty )
			result.emplace_back(std::move(tmp));

		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(view);
	}
	return result;
}

} //namespace libgs

namespace std
{

template <libgs::concepts::char_type CharT, template<typename,typename...> class Container, typename...Args>
class LIBGS_CORE_TAPI formatter<libgs::basic_string_container<CharT,Container,Args...>, CharT>
{
	template <char...Chars>
	static constexpr auto s_str = libgs::s_str<CharT,Chars...>;

public:
	auto format(const libgs::basic_string_container<CharT,Container,Args...> &container, auto &context) const
	{
		if( container.empty() )
			return m_formatter.format(s_str<'[',']'>, context);

		std::basic_string<CharT> buf = s_str<'['>;
		for(auto &str : container)
			buf += s_str<'\''> + str + s_str<'\'',',',' '>;

		buf.erase(buf.size() - 2, 2);
		buf += s_str<']'>;
		return m_formatter.format(buf, context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};

} //namespace std


#endif //LIBGS_CORE_DETAIL_STRING_LIST_H
