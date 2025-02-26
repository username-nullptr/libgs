
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

#ifndef LIBGS_CORE_CXX_FSTREAM_CONCEPTS_H
#define LIBGS_CORE_CXX_FSTREAM_CONCEPTS_H

#include <libgs/core/cxx/string_concepts.h>

namespace libgs
{

template <concepts::char_type, typename>
struct is_basic_fstream : std::false_type {};

template <concepts::char_type CharT>
struct is_basic_fstream<CharT,std::basic_fstream<CharT>> : std::true_type {};

template <concepts::char_type CharT, typename T>
constexpr bool is_basic_fstream_v = is_basic_fstream<CharT,T>::value;

template <typename T>
using is_char_fstream = is_basic_fstream<char,T>;

template <typename T>
constexpr bool is_char_fstream_v = is_char_fstream<T>::value;

template <typename T>
using is_wchar_fstream = is_basic_fstream<wchar_t,T>;

template <typename T>
constexpr bool is_wchar_fstream_v = is_wchar_fstream<T>::value;

template <typename T>
using is_char8_fstream = is_basic_fstream<char8_t,T>;

template <typename T>
constexpr bool is_char8_fstream_v = is_char8_fstream<T>::value;

template <typename T>
using is_char16_fstream = is_basic_fstream<char16_t,T>;

template <typename T>
constexpr bool is_char16_fstream_v = is_char16_fstream<T>::value;

template <typename T>
using is_char32_fstream = is_basic_fstream<char32_t,T>;

template <typename T>
constexpr bool is_char32_fstream_v = is_char32_fstream<T>::value;

template <typename T>
struct is_fstream : std::disjunction <
	is_char_fstream<T>, is_wchar_fstream<T>,
	is_char8_fstream<T>, is_char16_fstream<T>, is_char32_fstream<T>
> {};

template <typename T>
constexpr bool is_fstream_v = is_fstream<T>::value;

template <concepts::char_type, typename>
struct is_basic_ofstream : std::false_type {};

template <concepts::char_type CharT>
struct is_basic_ofstream<CharT,std::basic_ofstream<CharT>> : std::true_type {};

template <concepts::char_type CharT, typename T>
constexpr bool is_basic_ofstream_v = is_basic_ofstream<CharT,T>::value;

template <typename T>
using is_char_ofstream = is_basic_ofstream<char,T>;

template <typename T>
constexpr bool is_char_ofstream_v = is_char_ofstream<T>::value;

template <typename T>
using is_wchar_ofstream = is_basic_ofstream<wchar_t,T>;

template <typename T>
constexpr bool is_wchar_ofstream_v = is_wchar_ofstream<T>::value;

template <typename T>
using is_char8_ofstream = is_basic_ofstream<char8_t,T>;

template <typename T>
constexpr bool is_char8_ofstream_v = is_char8_ofstream<T>::value;

template <typename T>
using is_char16_ofstream = is_basic_ofstream<char16_t,T>;

template <typename T>
constexpr bool is_char16_ofstream_v = is_char16_ofstream<T>::value;

template <typename T>
using is_char32_ofstream = is_basic_ofstream<char32_t,T>;

template <typename T>
constexpr bool is_char32_ofstream_v = is_char32_ofstream<T>::value;

template <typename T>
struct is_ofstream : std::disjunction <
	is_char_ofstream<T>, is_wchar_ofstream<T>,
	is_char8_ofstream<T>, is_char16_ofstream<T>, is_char32_ofstream<T>
> {};

template <typename T>
constexpr bool is_ofstream_v = is_ofstream<T>::value;

template <concepts::char_type, typename>
struct is_basic_ifstream : std::false_type {};

template <concepts::char_type CharT>
struct is_basic_ifstream<CharT,std::basic_ifstream<CharT>> : std::true_type {};

template <concepts::char_type CharT, typename T>
constexpr bool is_basic_ifstream_v = is_basic_ifstream<CharT,T>::value;

template <typename T>
using is_char_ifstream = is_basic_ifstream<char,T>;

template <typename T>
constexpr bool is_char_ifstream_v = is_char_ifstream<T>::value;

template <typename T>
using is_wchar_ifstream = is_basic_ifstream<wchar_t,T>;

template <typename T>
constexpr bool is_wchar_ifstream_v = is_wchar_ifstream<T>::value;

template <typename T>
using is_char8_ifstream = is_basic_ifstream<char8_t,T>;

template <typename T>
constexpr bool is_char8_ifstream_v = is_char8_ifstream<T>::value;

template <typename T>
using is_char16_ifstream = is_basic_ifstream<char16_t,T>;

template <typename T>
constexpr bool is_char16_ifstream_v = is_char16_ifstream<T>::value;

template <typename T>
using is_char32_ifstream = is_basic_ifstream<char32_t,T>;

template <typename T>
constexpr bool is_char32_ifstream_v = is_char32_ifstream<T>::value;

template <typename T>
struct is_ifstream : std::disjunction<is_char_ifstream<T>, is_wchar_ifstream<T>> {};

template <typename T>
constexpr bool is_ifstream_v = is_ifstream<T>::value;

namespace concepts
{

template <typename T, typename CharT>
concept basic_fstream =
	is_basic_fstream_v<CharT,T> or
	is_basic_ofstream_v<CharT,T> or
	is_basic_ifstream_v<CharT,T>;

template <typename T, typename CharT>
concept basic_fstream_wkn = basic_fstream<std::remove_reference_t<T>,CharT>;

template <typename T>
concept char_fstream = basic_fstream<T,char>;

template <typename T>
concept char_fstream_wkn = char_fstream<std::remove_reference_t<T>>;

template <typename T>
concept wchar_fstream = basic_fstream<T,wchar_t>;

template <typename T>
concept wchar_fstream_wkn = wchar_fstream<std::remove_reference_t<T>>;

template <typename T>
concept char8_fstream = basic_fstream<T,char8_t>;

template <typename T>
concept char8_fstream_wkn = char8_fstream<std::remove_reference_t<T>>;

template <typename T>
concept char16_fstream = basic_fstream<T,char16_t>;

template <typename T>
concept char16_fstream_wkn = char16_fstream<std::remove_reference_t<T>>;

template <typename T>
concept char32_fstream = basic_fstream<T,char32_t>;

template <typename T>
concept char32_fstream_wkn = char32_fstream<std::remove_reference_t<T>>;

template <typename T>
concept fstream =
	char_fstream<T> or wchar_fstream<T> or
	char8_fstream<T> or char16_fstream<T> or char32_fstream<T>;

template <typename T>
concept fstream_wkn = fstream<std::remove_reference_t<T>>;

}} //namespace libgs


#endif //LIBGS_CORE_CXX_FSTREAM_CONCEPTS_H