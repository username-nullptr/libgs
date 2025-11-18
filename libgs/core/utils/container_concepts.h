
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_UTILS_CONTAINER_CONCEPTS_H
#define LIBGS_CORE_UTILS_CONTAINER_CONCEPTS_H

#include <libgs/core/cxx/string_concepts.h>

namespace libgs
{

template <typename T, typename Iter>
struct is_iterator
{
	static constexpr bool value =
	requires(decltype(std::declval<Iter>()) begin, decltype(std::declval<Iter>()) end)
	{
		++begin == --end;
		T{ *begin };
		T{ *end   };
	};
};

template <typename T, typename Iter>
constexpr bool is_iterator_v = is_iterator<T,Iter>::value;


template <typename Iter>
struct is_any_iterator
{
	static constexpr bool value =
	requires(decltype(std::declval<Iter>()) begin, decltype(std::declval<Iter>()) end)
	{
		++begin == --end;
		*begin;
		*end;
	};
};

template <typename Iter>
constexpr bool is_any_iterator_v = is_any_iterator<Iter>::value;

namespace concepts
{

template <typename T, typename Iter>
concept iterator = is_iterator_v<T,Iter>;

template <typename T, typename Iter>
concept iterator_p = iterator<std::remove_cvref_t<T>,Iter>;

template <typename Iter>
concept any_iterator = is_any_iterator_v<Iter>;

template <typename Iter>
concept any_iterator_p = any_iterator<std::remove_cvref_t<Iter>>;

} //namespace concepts

template <typename>
struct get_iterator;

template <concepts::any_iterator T>
struct get_iterator<T> {
	using type = T;
};

template <typename T, size_t N>
struct get_iterator<T[N]> {
	using type = decltype(std::declval<T[N]>() + 1);
};

template <typename T>
struct get_iterator<T[]> {
	using type = decltype(std::declval<T[]>() + 1);
};

template <typename T, size_t N>
struct get_iterator<T(&)[N]> : get_iterator<T[N]> {};

template <typename T>
struct get_iterator<T(&)[]> : get_iterator<T[]> {};

template <typename T>
using get_iterator_t = typename get_iterator<T>::type;

template <typename>
struct can_get_iterator : std::false_type {};

template <concepts::any_iterator T>
struct can_get_iterator<T> : std::true_type {};

template <typename T, size_t N>
struct can_get_iterator<T[N]> : std::true_type {};

template <typename T>
struct can_get_iterator<T[]> : std::true_type {};

template <typename T, size_t N>
struct can_get_iterator<T(&)[N]> : can_get_iterator<T[N]> {};

template <typename T>
struct can_get_iterator<T(&)[]> : can_get_iterator<T[]> {};

template <typename T>
constexpr bool can_get_iterator_v = can_get_iterator<T>::value;

template <typename, typename>
struct can_get_type_iterator : std::false_type {};

template <typename T, concepts::iterator<T> Iter>
struct can_get_type_iterator<T,Iter> : std::true_type {};

template <typename T, size_t N>
struct can_get_type_iterator<T,T[N]> : std::true_type {};

template <typename T>
struct can_get_type_iterator<T,T[]> : std::true_type {};

template <typename T, size_t N>
struct can_get_type_iterator<T,T(&)[N]> : can_get_type_iterator<T,T[N]> {};

template <typename T>
struct can_get_type_iterator<T,T(&)[]> : can_get_type_iterator<T,T[]> {};

template <typename T, typename Iter>
constexpr bool can_get_type_iterator_v = can_get_type_iterator<T,Iter>::value;

} //namespace libgs::concepts


#endif //LIBGS_CORE_UTILS_CONTAINER_CONCEPTS_H