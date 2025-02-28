
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

#ifndef LIBGS_CORE_CXX_STRING_CONCEPTS_H
#define LIBGS_CORE_CXX_STRING_CONCEPTS_H

#include <libgs/core/cxx/concepts.h>

namespace libgs
{

template <typename T>
using is_char = std::is_same<std::remove_const_t<T>, char>;

template <typename T>
constexpr bool is_char_v = is_char<T>::value;

template <typename T>
using is_wchar = std::is_same<std::remove_const_t<T>,wchar_t>;

template <typename T>
constexpr bool is_wchar_v = is_wchar<T>::value;

template <typename T>
using is_char8 = std::is_same<std::remove_const_t<T>, char8_t>;

template <typename T>
constexpr bool is_char8_v = is_char8<T>::value;

template <typename T>
using is_char16 = std::is_same<std::remove_const_t<T>, char16_t>;

template <typename T>
constexpr bool is_char16_v = is_char16<T>::value;

template <typename T>
using is_char32 = std::is_same<std::remove_const_t<T>, char32_t>;

template <typename T>
constexpr bool is_char32_v = is_char32<T>::value;

namespace concepts
{

template <typename T>
concept char_type =
	is_char_v<T> or  is_wchar_v<T> or
	is_char8_v<T> or is_char16_v<T> or is_char32_v<T>;

} //namespace concepts

template <concepts::char_type, typename>
struct is_basic_char_array : std::false_type {};

template <concepts::char_type CharT, concepts::char_type T, size_t N>
struct is_basic_char_array<CharT, T[N]> :
	std::is_same<std::remove_const_t<CharT>, std::remove_const_t<T>> {};

template <concepts::char_type CharT, concepts::char_type T>
struct is_basic_char_array<CharT, T[]> :
	std::is_same<std::remove_const_t<CharT>, std::remove_const_t<T>> {};

template <concepts::char_type CharT, concepts::char_type T, size_t N>
struct is_basic_char_array<CharT, T(&)[N]> : is_basic_char_array<CharT, T[N]> {};

template <concepts::char_type CharT, concepts::char_type T>
struct is_basic_char_array<CharT, T(&)[]> : is_basic_char_array<CharT, T[]> {};

template <concepts::char_type CharT, typename T>
constexpr bool is_basic_char_array_v = is_basic_char_array<CharT,T>::value;

template <typename CharT>
using is_char_array = is_basic_char_array<char, CharT>;

template <typename CharT>
constexpr bool is_char_array_v = is_char_array<CharT>::value;

template <typename CharT>
using is_wchar_array = is_basic_char_array<wchar_t, CharT>;

template <typename CharT>
constexpr bool is_wchar_array_v = is_wchar_array<CharT>::value;

template <typename CharT>
using is_char8_array = is_basic_char_array<char8_t, CharT>;

template <typename CharT>
constexpr bool is_char8_array_v = is_char8_array<CharT>::value;

template <typename CharT>
using is_char16_array = is_basic_char_array<char16_t, CharT>;

template <typename CharT>
constexpr bool is_char16_array_v = is_char16_array<CharT>::value;

template <typename CharT>
using is_char32_array = is_basic_char_array<char32_t, CharT>;

template <typename CharT>
constexpr bool is_char32_array_v = is_char32_array<CharT>::value;

template <typename CharT>
using is_any_char_array = std::disjunction <
	is_basic_char_array<char    , CharT>,
	is_basic_char_array<wchar_t , CharT>,
	is_basic_char_array<char8_t , CharT>,
	is_basic_char_array<char16_t, CharT>,
	is_basic_char_array<char32_t, CharT>
>;

template <typename CharT>
constexpr bool is_any_char_array_v = is_any_char_array<CharT>::value;

template <typename, concepts::char_type>
struct is_basic_std_string : std::false_type {};

template <concepts::char_type CharT, typename...Args>
struct is_basic_std_string<std::basic_string<CharT,Args...>,CharT> : std::true_type {};

template <typename T, concepts::char_type CharT>
constexpr bool is_basic_std_string_v = is_basic_std_string<T,CharT>::value;

template <typename T>
using is_std_string = is_basic_std_string<T,char>;

template <typename T>
constexpr bool is_std_string_v = is_std_string<T>::value;

template <typename T>
using is_std_wstring = is_basic_std_string<T,wchar_t>;

template <typename T>
constexpr bool is_std_wstring_v = is_std_wstring<T>::value;

template <typename T>
using is_std_u8string = is_basic_std_string<T,char8_t>;

template <typename T>
constexpr bool is_std_u8string_v = is_std_u8string<T>::value;

template <typename T>
using is_std_u16string = is_basic_std_string<T,char16_t>;

template <typename T>
constexpr bool is_std_u16string_v = is_std_u16string<T>::value;

template <typename T>
using is_std_u32string = is_basic_std_string<T,char32_t>;

template <typename T>
constexpr bool is_std_u32string_v = is_std_u32string<T>::value;

template <typename, concepts::char_type>
struct is_basic_std_string_view : std::false_type {};

template <concepts::char_type CharT, typename...Args>
struct is_basic_std_string_view<std::basic_string<CharT,Args...>,CharT> : std::true_type {};

template <typename T, concepts::char_type CharT>
constexpr bool is_basic_std_string_view_v = is_basic_std_string_view<T,CharT>::value;

template <typename T>
using is_std_string_view = is_basic_std_string_view<T,char>;

template <typename T>
constexpr bool is_std_string_view_v = is_std_string_view<T>::value;

template <typename T>
using is_std_wstring_view = is_basic_std_string_view<T,wchar_t>;

template <typename T>
constexpr bool is_std_wstring_view_v = is_std_wstring_view<T>::value;

template <typename T>
using is_std_u8string_view = is_basic_std_string_view<T,char8_t>;

template <typename T>
constexpr bool is_std_u8string_view_v = is_std_u8string_view<T>::value;

template <typename T>
using is_std_u16string_view = is_basic_std_string_view<T,char16_t>;

template <typename T>
constexpr bool is_std_u16string_view_v = is_std_u16string_view<T>::value;

template <typename T>
using is_std_u32string_view = is_basic_std_string_view<T,char32_t>;

template <typename T>
constexpr bool is_std_u32string_view_v = is_std_u32string_view<T>::value;

template <typename, concepts::char_type>
struct is_basic_std_string;

template <typename T, concepts::char_type CharT>
struct is_basic_string
{
	using rcr_T = std::remove_cvref_t<T>;

	static constexpr bool value =
		is_basic_std_string_v<rcr_T,CharT> or
		is_basic_std_string_v<rcr_T,CharT> or
		std::is_same_v<rcr_T, const CharT*> or
		std::is_same_v<rcr_T, CharT*> or
		is_basic_char_array_v<CharT, rcr_T>;
};

template <typename T, concepts::char_type CharT>
constexpr bool is_basic_string_v = is_basic_string<T,CharT>::value;

template <typename T>
using is_char_string = is_basic_string<T,char>;

template <typename T>
constexpr bool is_char_string_v = is_char_string<T>::value;

template <typename T>
using is_wchar_string = is_basic_string<T,wchar_t>;

template <typename T>
constexpr bool is_wchar_string_v = is_wchar_string<T>::value;

template <typename T>
using is_char8_string = is_basic_string<T,char8_t>;

template <typename T>
constexpr bool is_char8_string_v = is_char8_string<T>::value;

template <typename T>
using is_char16_string = is_basic_string<T,char16_t>;

template <typename T>
constexpr bool is_char16_string_v = is_char16_string<T>::value;

template <typename T>
using is_char32_string = is_basic_string<T,char32_t>;

template <typename T>
constexpr bool is_char32_string_v = is_char32_string<T>::value;

template <typename T>
struct is_string : std::disjunction <
	is_char_string<T>, is_wchar_string<T>,
	is_char8_string<T>, is_char16_string<T>, is_char32_string<T>
> {};

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

namespace concepts
{

template <typename T, typename CharT>
concept basic_string_type = is_basic_string_v<T,CharT>;

template <typename T>
concept char_string_type = is_char_string_v<T>;

template <typename T>
concept wchar_string_type = is_wchar_string_v<T>;

template <typename T>
concept char8_string_type = is_char8_string_v<T>;

template <typename T>
concept char16_string_type = is_char16_string_v<T>;

template <typename T>
concept char32_string_type = is_char32_string_v<T>;

template <typename T>
concept string_type = is_string_v<T>;

template <typename T, typename CharT>
concept weak_basic_string_type =
	basic_string_type<T,CharT> or std::is_same_v<std::remove_cvref_t<T>,CharT>;

template <typename T>
concept weak_char_string_type = weak_basic_string_type<T,char>;

template <typename T>
concept weak_wchar_string_type = weak_basic_string_type<T,wchar_t>;

template <typename T>
concept weak_char8_string_type = weak_basic_string_type<T,char8_t>;

template <typename T>
concept weak_char16_string_type = weak_basic_string_type<T,char16_t>;

template <typename T>
concept weak_char32_string_type = weak_basic_string_type<T,char32_t>;

template <typename T>
concept weak_string_type =
	weak_char_string_type<T> or weak_wchar_string_type<T> or
	weak_char8_string_type<T> or weak_char16_string_type<T> or weak_char32_string_type<T>;

}} //namespace libgs


#endif //LIBGS_CORE_CXX_STRING_CONCEPTS_H