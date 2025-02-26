
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

template <typename T0, typename T1>
using is_dsame = std::is_same<std::decay_t<T0>, T1>;

template <typename T0, typename T1>
constexpr bool is_dsame_v = is_dsame<std::decay_t<T0>, T1>::value;

namespace concepts
{

template <typename T, typename Iter>
concept iterator = requires(decltype(std::declval<Iter>()) begin, decltype(std::declval<Iter>()) end)
{
	++begin == --end;
	T{ *begin };
	T{ *end   };
};

template <typename Iter>
concept any_iterator = requires(decltype(std::declval<Iter>()) begin, decltype(std::declval<Iter>()) end)
{
	++begin == --end;
	*begin;
	*end;
};

} //namespace concepts

template <typename>
struct match_iterator;

template <concepts::any_iterator T>
struct match_iterator<T> {
	using type = T;
};

template <typename T, size_t N>
struct match_iterator<T[N]> {
	using type = decltype(std::declval<T[N]>() + 1);
};

template <typename T>
struct match_iterator<T[]> {
	using type = decltype(std::declval<T[]>() + 1);
};

template <typename T, size_t N>
struct match_iterator<T(&)[N]> : match_iterator<T[N]> {};

template <typename T>
struct match_iterator<T(&)[]> : match_iterator<T[]> {};

template <typename T>
using match_iterator_t = typename match_iterator<T>::type;

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

template <typename T, typename Base>
concept base_of = std::is_base_of_v<Base,T>;

template <typename T, typename...Args>
concept all_types = std::conjunction_v<std::is_same<T,std::remove_cvref_t<Args>>...>;

template <typename T, typename...Args>
concept container_params = []() consteval -> bool
{
	if constexpr( sizeof...(Args) == 0 )
		return false;
	else if constexpr( constructible<T,Args...> )
		return true;

	else if constexpr( sizeof...(Args) == 1 )
	{
		using container_t = std::tuple_element_t<0,std::tuple<Args...>>;
		return requires(const container_t &container)
		{
			T{ *std::begin(container) };
			T{ *std::end  (container) };
		};
	}
	else if constexpr( sizeof...(Args) == 2 )
	{
		using tuple_t = std::tuple<Args...>;
		using begin_t = match_iterator_t<std::tuple_element_t<0,tuple_t>>;
		using end_t = match_iterator_t<std::tuple_element_t<1,tuple_t>>;
		return iterator<T,begin_t> and iterator<T,end_t>;
	}
	return false;
}();


}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_CONCEPTS_H
