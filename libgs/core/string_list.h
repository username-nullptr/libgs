
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
#include <set>

namespace libgs { namespace concepts
{

template <typename T, typename CharT, template<typename,typename...> class Container, typename...Args>
concept string_list_iterator =
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::iterator> and
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::const_iterator> and
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::reverse_iterator> and
	std::is_same_v<T, typename Container<std::basic_string<CharT>,Args...>::const_reverse_iterator>;

} //namespace concepts

template <concepts::character CharT, template<typename,typename...> class Container, typename...Args>
class LIBGS_CORE_TAPI basic_string_container : public Container<std::basic_string<CharT>,Args...>
{
public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using base_t = Container<string_t,Args...>;
	using base_t::base_t;

	constexpr char_t space = 0x20;

public:
	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(const Text &splits = space);

	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(size_t index, size_t length, const Text &splits = space);

	template <concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] string_t join(size_t index, const Text &splits = space);

	template <concepts::string_list_iterator<CharT,Container,Args...> Iter,
			  concepts::text_p<CharT> Text = char_t>
	[[nodiscard]] static string_t join (
		Iter begin, Iter end, const Text &splits = space
	);

	template <concepts::text_p<CharT> Str = char_t>
	[[nodiscard]] static basic_string_container from_string (
		concepts::string_p<char_t> auto &&str, const Str &splits = space,
		bool ignore_empty = true
	);
};

template <concepts::character CharT, typename...Args>
using basic_string_vector = basic_string_container<CharT,std::vector,Args...>;

using string_vector    = basic_string_vector<char    >;
using wstring_vector   = basic_string_vector<wchar_t >;
using u8string_vector  = basic_string_vector<char8_t >;
using u16string_vector = basic_string_vector<char16_t>;
using u32string_vector = basic_string_vector<char32_t>;

template <concepts::character CharT, typename...Args>
using basic_string_deque = basic_string_container<CharT,std::deque,Args...>;

using string_deque    = basic_string_deque<char    >;
using wstring_deque   = basic_string_deque<wchar_t >;
using u8string_deque  = basic_string_deque<char8_t >;
using u16string_deque = basic_string_deque<char16_t>;
using u32string_deque = basic_string_deque<char32_t>;

template <concepts::character CharT, typename...Args>
using basic_string_list = basic_string_container<CharT,std::list,Args...>;

using string_list    = basic_string_list<char    >;
using wstring_list   = basic_string_list<wchar_t >;
using u8string_list  = basic_string_list<char8_t >;
using u16string_list = basic_string_list<char16_t>;
using u32string_list = basic_string_list<char32_t>;

template <concepts::character CharT, typename...Args>
using basic_string_set = basic_string_container<CharT,std::set,Args...>;

using string_set    = basic_string_set<char    >;
using wstring_set   = basic_string_set<wchar_t >;
using u8string_set  = basic_string_set<char8_t >;
using u16string_set = basic_string_set<char16_t>;
using u32string_set = basic_string_set<char32_t>;

} //namespace libgs
#include <libgs/core/detail/string_list.h>


#endif //LIBGS_CORE_STRING_LIST_H
