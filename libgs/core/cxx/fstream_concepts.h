
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

template <concepts::character, typename>
struct is_fstream : std::false_type {};

template <concepts::character CharT>
struct is_fstream<CharT,std::basic_fstream<CharT>> : std::true_type {};

template <concepts::character CharT, typename T>
constexpr bool is_fstream_v = is_fstream<CharT,T>::value;

template <typename T>
struct is_any_fstream : std::disjunction <
	is_fstream<char,T>, is_fstream<wchar_t,T>,
	is_fstream<char8_t,T>, is_fstream<char16_t,T>, is_fstream<char32_t,T>
> {};

template <typename T>
constexpr bool is_any_fstream_v = is_any_fstream<T>::value;

template <concepts::character, typename>
struct is_ofstream : std::false_type {};

template <concepts::character CharT>
struct is_ofstream<CharT,std::basic_ofstream<CharT>> : std::true_type {};

template <concepts::character CharT, typename T>
constexpr bool is_ofstream_v = is_ofstream<CharT,T>::value;

template <typename T>
struct is_any_ofstream : std::disjunction <
	is_ofstream<char,T>, is_ofstream<wchar_t,T>,
	is_ofstream<char8_t,T>, is_ofstream<char16_t,T>, is_ofstream<char32_t,T>
> {};

template <typename T>
constexpr bool is_any_ofstream_v = is_any_ofstream<T>::value;

template <concepts::character, typename>
struct is_ifstream : std::false_type {};

template <concepts::character CharT>
struct is_ifstream<CharT,std::basic_ifstream<CharT>> : std::true_type {};

template <concepts::character CharT, typename T>
constexpr bool is_ifstream_v = is_ifstream<CharT,T>::value;

template <typename T>
struct is_any_ifstream : std::disjunction <
	is_ifstream<char,T>, is_ifstream<wchar_t,T>,
	is_ifstream<char8_t,T>, is_ifstream<char16_t,T>, is_ifstream<char32_t,T>
> {};

template <typename T>
constexpr bool is_any_ifstream_v = is_any_ifstream<T>::value;

namespace concepts
{

template <typename T, typename CharT>
concept fstream =
	is_fstream_v<CharT,T> or
	is_ofstream_v<CharT,T> or
	is_ifstream_v<CharT,T>;

template <typename T, typename CharT>
concept fstream_p = fstream<std::remove_reference_t<T>,CharT>;

template <typename T>
concept any_fstream =
	fstream<T,char> or fstream<T,wchar_t> or
	fstream<T,char8_t> or fstream<T,char16_t> or fstream<T,char32_t>;

template <typename T>
concept any_fstream_p = any_fstream<std::remove_reference_t<T>>;

}} //namespace libgs


#endif //LIBGS_CORE_CXX_FSTREAM_CONCEPTS_H