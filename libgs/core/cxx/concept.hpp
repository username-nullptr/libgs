
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

#ifndef LIBGS_CORE_CXX_CONCEPT_H
#define LIBGS_CORE_CXX_CONCEPT_H

#include <concepts>
#include <string>

namespace libgs
{

template <typename T>
using is_float = std::is_floating_point<T>;

template <typename T>
constexpr bool is_float_v = is_float<T>::value;

template <typename T>
using is_bool = std::is_same<T, bool>;

template <typename T>
constexpr bool is_bool_v = is_bool<T>::value;

template <typename T>
concept concept_number_type = std::is_arithmetic_v<T>;

template <typename T>
concept concept_enum_type = std::is_enum_v<T>;

template <typename T>
using is_char = std::is_same<T,char>;

template <typename T>
constexpr bool is_char_v = is_char<T>::value;

template <typename T>
using is_wchar = std::is_same<T,wchar_t>;

template <typename T>
constexpr bool is_wchar_v = is_wchar<T>::value;

template <typename T>
concept concept_char_type = is_char_v<T> or is_wchar_v<T>;

template <typename T0, typename T1>
using is_dsame = std::is_same<std::decay_t<T0>, T1>;

template <typename T0, typename T1>
constexpr bool is_dsame_v = libgs::is_dsame<std::decay_t<T0>, T1>::value;

template <typename CharT, typename T>
struct is_basic_string
{
	static constexpr bool value =
		is_dsame_v<T, std::basic_string<CharT>> or
		std::is_same_v<T, const CharT*> or
		std::is_same_v<T, CharT*>;
};

template <typename CharT, typename T>
constexpr bool is_basic_string_v = is_basic_string<CharT,T>::value;

template <typename CharT, typename T>
concept concept_basic_string_type = is_basic_string_v<CharT,T>;

template <typename T>
using is_char_string = is_basic_string<char,T>;

template <typename T>
constexpr bool is_char_string_v = is_char_string<T>::value;

template <typename T>
concept concept_char_string_type = is_char_string_v<T>;

template <typename T>
using is_wchar_string = is_basic_string<wchar_t,T>;

template <typename T>
constexpr bool is_wchar_string_v = is_wchar_string<T>::value;

template <typename T>
concept concept_wchar_string_type = is_wchar_string_v<T>;

template <typename T>
struct is_string {
	static constexpr bool value = is_char_string_v<T> or is_wchar_string_v<T>;
};

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

template <typename T>
concept concept_string_type = is_string_v<T>;

} //namespace libgs


#endif //LIBGS_CORE_CXX_CONCEPT_H
