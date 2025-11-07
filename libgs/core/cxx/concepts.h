
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
#include <chrono>

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

template <typename>
struct is_tuple : std::false_type {};

template <typename... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type {};

template <typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

template <typename T0, typename T1>
using is_dsame = std::is_same<std::decay_t<T0>, T1>;

template <typename T0, typename T1>
constexpr bool is_dsame_v = is_dsame<std::decay_t<T0>, T1>::value;

template <typename T>
struct is_time_point : std::false_type {};

template <typename Clock, typename Duration>
struct is_time_point<std::chrono::time_point<Clock, Duration>> : std::true_type {};

template <typename T>
constexpr bool is_time_point_v = is_time_point<T>::value;

template <typename T>
struct is_duration : std::false_type {};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

template <typename T>
constexpr bool is_duration_v = is_duration<T>::value;

template <typename T>
struct is_time : std::disjunction<is_time_point<T>, is_duration<T>> {};

template <typename T>
constexpr bool is_time_v = is_time<T>::value;

namespace concepts
{

template <typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept arithmetic_p = arithmetic<std::remove_cvref_t<T>>;

template <typename T>
concept integral = std::integral<T>;

template <typename T>
concept integral_p = integral<std::remove_cvref_t<T>>;

template <typename T>
concept floating = std::floating_point<T>;

template <typename T>
concept floating_p = floating<std::remove_cvref_t<T>>;

template <typename T>
concept enumerate = std::is_enum_v<T>;

template <typename T>
concept enumerate_p = enumerate<std::remove_cvref_t<T>>;

template <typename T>
concept rvalue_reference = std::is_rvalue_reference_v<T>;

template <typename Func>
concept function = is_function_v<Func>;

template <typename T>
concept void_function = is_void_func_v<T>;

template <typename Func, typename...Args>
concept callable = requires(Func &&func, Args&&...args) {
	func(std::forward<Args>(args)...);
};

template <typename Func, typename Res, typename...Args>
concept callable_ret = requires(Func &&func, Args&&...args) {
	{ func(std::forward<Args>(args)...) } -> std::same_as<Res>;
};

template <typename Func, typename Res, typename...Args>
concept callable_noret = requires(Func &&func, Args&&...args) {
	requires not std::is_same_v<decltype(func(std::forward<Args>(args)...)), Res>;
};

template <typename Func, typename...Args>
concept callable_void = callable_ret<Func, void, Args...>;

template <typename Func, typename...Args>
concept callable_novoid = requires(Func &&func, Args&&...args) {
	requires not std::is_void_v<decltype(func(std::forward<Args>(args)...))>;
};

template <typename Func>
concept std_func_temp = requires(Func *func) {
	std::function<Func>(func);
};

template <typename Struct, typename...Args>
concept constructible = std::constructible_from<Struct,Args...>;

template <typename L, typename R>
concept assignable = std::assignable_from<L,R>;

template <typename T>
concept copyable = std::copyable<T>;

template <typename T>
concept copy_constructible = std::copy_constructible<T>;

template <typename T>
concept movable = std::movable<T>;

template <typename T>
concept move_constructible = std::move_constructible<T>;

template <typename T>
concept copymovable = copyable<T> and movable<T>;

template <typename T>
concept copymove_constructible = copy_constructible<T> and move_constructible<T>;

template <typename T>
concept copy_or_movable = copyable<T> or movable<T>;

template <typename T>
concept copy_or_move_constructible = copy_constructible<T> or move_constructible<T>;

template <typename T>
concept optional_value = copy_or_move_constructible<T> and concepts::constructible<T>;

template <typename T>
concept optional_value_p = optional_value<std::remove_cvref_t<T>>;

template <typename T, typename Base>
concept base_of = std::is_base_of_v<Base,T>;

template <typename T, typename...Args>
concept all_types = std::conjunction_v<std::is_same<T,std::remove_cvref_t<Args>>...>;

template <typename T>
concept time_point = is_time_point_v<T>;

template <typename T>
concept time_point_p = time_point<std::remove_cvref_t<T>>;

template <typename T>
concept duration = is_duration_v<T>;

template <typename T>
concept duration_p = duration<std::remove_cvref_t<T>>;

template <typename T>
concept time = is_time_v<T>;

template <typename T>
concept time_p = time<std::remove_cvref_t<T>>;

}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_CONCEPTS_H
