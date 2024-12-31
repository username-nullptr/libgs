
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

#ifndef LIBGS_CORE_CXX_CONCEPTS_H
#define LIBGS_CORE_CXX_CONCEPTS_H

#include <libgs/core/cxx/function_traits.h>
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
using is_char = std::is_same<std::remove_const_t<T>, char>;

template <typename T>
constexpr bool is_char_v = is_char<T>::value;

template <typename T>
using is_wchar = std::is_same<std::remove_const_t<T>,wchar_t>;

template <typename T>
constexpr bool is_wchar_v = is_wchar<T>::value;

namespace concepts
{

template <typename T>
concept char_type = is_char_v<T> or is_wchar_v<T>;

} //namespace concepts

template <typename T0, typename T1>
using is_dsame = std::is_same<std::decay_t<T0>, T1>;

template <typename T0, typename T1>
constexpr bool is_dsame_v = is_dsame<std::decay_t<T0>, T1>::value;

template <concepts::char_type, typename>
struct is_basic_char_array : std::false_type {};

template <concepts::char_type CharT, concepts::char_type T, size_t N>
struct is_basic_char_array<CharT, T[N]> :
	std::is_same<std::remove_const_t<CharT>, std::remove_const_t<T>> {};

template <concepts::char_type CharT, concepts::char_type T>
struct is_basic_char_array<CharT, T[]> :
	std::is_same<std::remove_const_t<CharT>, std::remove_const_t<T>> {};

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

template <typename T, concepts::char_type CharT>
struct is_basic_string
{
	using nc_T = std::remove_cvref_t<T>;
	using nc_CharT = std::remove_const_t<CharT>;

	static constexpr bool value =
		std::is_same_v<nc_T, std::basic_string<nc_CharT>> or
		std::is_same_v<nc_T, std::basic_string_view<nc_CharT>> or
		std::is_same_v<nc_T, const nc_CharT*> or
		std::is_same_v<nc_T, nc_CharT*> or
		is_basic_char_array_v<nc_CharT, nc_T>;
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
struct is_string : std::disjunction<is_char_string<T>, is_wchar_string<T>> {};

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

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
struct is_fstream : std::disjunction<is_char_fstream<T>, is_wchar_fstream<T>> {};

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
struct is_ofstream : std::disjunction<is_char_ofstream<T>, is_wchar_ofstream<T>> {};

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
struct is_ifstream : std::disjunction<is_char_ifstream<T>, is_wchar_ifstream<T>> {};

template <typename T>
constexpr bool is_ifstream_v = is_ifstream<T>::value;

namespace concepts
{

template <typename T>
concept number_type = std::is_arithmetic_v<T>;

template <typename T>
concept integral_type = std::integral<T>;

template <typename T>
concept float_type = std::floating_point<T>;

template <typename T>
concept enum_type = std::is_enum_v<T>;

template <typename T, typename CharT>
concept basic_string_type = is_basic_string_v<T,CharT>;

template <typename T>
concept char_string_type = is_char_string_v<T>;

template <typename T>
concept wchar_string_type = is_wchar_string_v<T>;

template <typename T>
concept string_type = is_string_v<T>;

template <typename T>
concept rvalue_reference = std::is_rvalue_reference_v<T>;

template <typename Func>
concept function = is_function_v<Func>;

template <typename T>
concept void_function = is_void_func_v<T>;

template <typename Func, typename Res, typename...Args>
concept callable_ret = requires(Func &&func, Args&&...args) {
	std::is_same_v<decltype(func(std::forward<Args>(args)...)), Res>;
};

template <typename Func, typename...Args>
concept callable_void = callable_ret<Func, void, Args...>;

template <typename Func, typename...Args>
concept callable = requires(Func &&func, Args&&...args) {
	func(std::forward<Args>(args)...);
};

template <typename Struct, typename...Args>
concept constructible = requires(Args&&...args) {
	Struct(std::forward<Args>(args)...);
};

template <typename T, typename Arg>
concept assignable = requires(Arg &&args) {
	std::declval<T>() = (std::forward<Arg>(args));
};

template <typename T>
concept copyable = std::copyable<T>;

template <typename T>
concept movable = std::movable<T>;

template <typename T>
concept copymovable = copyable<T> and movable<T>;

template <typename T, typename Base>
concept base_of = std::is_base_of_v<Base,T>;

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
concept fstream = char_fstream<T> or wchar_fstream<T>;

template <typename T>
concept fstream_wkn = fstream<std::remove_reference_t<T>>;

}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_CONCEPTS_H
