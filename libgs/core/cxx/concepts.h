
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
constexpr bool is_dsame_v = libgs::is_dsame<std::decay_t<T0>, T1>::value;

template <concepts::char_type, typename>
struct is_basic_char_array : std::false_type {};

template <concepts::char_type CharT, concepts::char_type T, size_t N>
struct is_basic_char_array<CharT, T[N]> {
	static constexpr bool value = std::is_same_v<std::remove_const_t<CharT>, std::remove_const_t<T>>;
};

template <concepts::char_type CharT, concepts::char_type T>
struct is_basic_char_array<CharT, T[]> {
	static constexpr bool value = std::is_same_v<std::remove_const_t<CharT>, std::remove_const_t<T>>;
};

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

template <concepts::char_type CharT, typename T>
struct is_basic_string
{
	using nc_T = std::remove_cvref_t<T>;
	using nc_CharT = std::remove_const_t<CharT>;

	static constexpr bool value =
		is_dsame_v<nc_T, std::basic_string<nc_CharT>> or
		is_dsame_v<nc_T, std::basic_string_view<nc_CharT>> or
		std::is_same_v<nc_T, const nc_CharT*> or
		std::is_same_v<nc_T, nc_CharT*> or
		is_basic_char_array_v<nc_CharT, nc_T>;
};

template <concepts::char_type CharT, typename T>
constexpr bool is_basic_string_v = is_basic_string<CharT,T>::value;

template <typename T>
using is_char_string = is_basic_string<char,T>;

template <typename T>
constexpr bool is_char_string_v = is_char_string<T>::value;

template <typename T>
using is_wchar_string = is_basic_string<wchar_t,T>;

template <typename T>
constexpr bool is_wchar_string_v = is_wchar_string<T>::value;

template <typename T>
struct is_string {
	static constexpr bool value = is_char_string_v<T> or is_wchar_string_v<T>;
};

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

template <typename Derived, typename Base>
struct crtp_derived { using type = Derived; };

template <typename Base>
struct crtp_derived<void,Base> { using type = Base; };

template <typename Derived, typename Base>
using crtp_derived_t = typename crtp_derived<Derived, Base>::type;

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

template <typename CharT, typename T>
concept basic_string_type = is_basic_string_v<CharT,T>;

template <typename T>
concept char_string_type = is_char_string_v<T>;

template <typename T>
concept wchar_string_type = is_wchar_string_v<T>;

template <typename T>
concept string_type = is_string_v<T>;

template <typename T>
concept rvalue = std::is_rvalue_reference_v<T>;

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

}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_CONCEPTS_H
