
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_DETAIL_STRING_CONTAINER_H
#define LIBGS_CORE_DETAIL_STRING_CONTAINER_H

namespace libgs
{

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::text_p<CharT> Text>
basic_string_container<CharT,Container,Args...>::string_t
basic_string_container<CharT,Container,Args...>::join(const Text &splits)
{
	string_t result;
	auto view = strtls::to_view(splits);

	for(auto &str : *this)
		result += str + string_t(view.data(), view.size());

	result.erase(result.size() - view.size(), view.size());
	return result;
}

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::text_p<CharT> Text>
basic_string_container<CharT,Container,Args...>::string_t
basic_string_container<CharT,Container,Args...>::join(size_t index, size_t length, const Text &splits)
{
	string_t result;
	auto view = strtls::to_view(splits);

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

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::text_p<CharT> Text>
basic_string_container<CharT,Container,Args...>::string_t
basic_string_container<CharT,Container,Args...>::join(size_t index, const Text &splits)
{
	return join(index, this->size(), splits);
}

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
template <concepts::text_p<CharT> Text>
basic_string_container<CharT,Container,Args...>
basic_string_container<CharT,Container,Args...>::from_string
(concepts::string_p<char_t> auto &&str, const Text &splits, bool ignore_empty)
{
	basic_string_container result;
	auto view = strtls::to_view(splits);

	if( view.empty() )
		return result;

	string_t strs(std::forward<decltype(str)>(str));
	strs += view;

	auto pos = strs.find(view);
	auto step = view.size();

	while( pos != std::basic_string<CharT>::npos )
	{
		if( auto tmp = strs.substr(0, pos); not strtls::trimmed(tmp).empty() or not ignore_empty )
			result.emplace_back(std::move(tmp));

		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(view);
	}
	return result;
}

} //namespace libgs

template <libgs::concepts::character CharT, template<typename,typename...> class Container, typename...Args>
struct LIBGS_CORE_TAPI std::formatter<libgs::basic_string_container<CharT,Container,Args...>, CharT>
{
	auto format(const libgs::basic_string_container<CharT,Container,Args...> &container, auto &context) const
	{
		if( container.empty() )
			return m_formatter.format(l_str(CharT,"[]"), context);

		std::basic_string<CharT> buf = l_str(CharT,"[");
		for(auto &str : container)
			buf += l_str(CharT,"'") + str + l_str(CharT,"', ");

		buf.erase(buf.size() - 2, 2);
		buf += l_str(CharT,"]");
		return m_formatter.format(buf, context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};


#endif //LIBGS_CORE_DETAIL_STRING_CONTAINER_H
