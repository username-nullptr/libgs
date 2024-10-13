
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_REMOVE_REPEAT_H
#define LIBGS_CORE_CXX_REMOVE_REPEAT_H

#include <variant>
#include <tuple>

namespace libgs
{

template<typename T, typename... Args>
struct has_tof_args : std::disjunction<std::is_same<T, Args>...> {};

template<typename T, typename... Args>
constexpr auto has_tof_args_v = has_tof_args<T, Args...>::value;

template<typename T>
struct remove_repeat; // The template type must be std::tuple<...> or std::variant<...>

template<>
struct remove_repeat<std::tuple<>> {
	using type = std::tuple<>;
};

template<typename...Args>
struct remove_repeat<std::tuple<Args...>>
{
	template<typename Tup0, typename Tup1>
	struct remove_repeat_helper;

	template<typename FArgs, typename...RArgs, typename...SaveArgs>
	struct remove_repeat_helper<std::tuple<FArgs, RArgs...>, std::tuple<SaveArgs...>>
	{
		using inn_type = std::conditional_t<has_tof_args_v<FArgs, SaveArgs...>, std::tuple<SaveArgs...>, std::tuple<FArgs, SaveArgs...>>;
		using type = typename remove_repeat_helper<std::tuple<RArgs...>, inn_type>::type;
	};

	template<typename...SaveArgs>
	struct remove_repeat_helper<std::tuple<>, std::tuple<SaveArgs...>> {
		using type = std::tuple<SaveArgs...>;
	};

	using type = typename remove_repeat_helper<std::tuple<Args...>, std::tuple<>>::type;
};

template<typename...Args>
struct remove_repeat<std::variant<Args...>>
{
	template<typename Tup0, typename Tup1>
	struct remove_repeat_helper;

	template<typename FArgs, typename...RArgs, typename...SaveArgs>
	struct remove_repeat_helper<std::variant<FArgs, RArgs...>, std::variant<SaveArgs...>>
	{
		using inn_type = std::conditional_t<has_tof_args_v<FArgs, SaveArgs...>, std::variant<SaveArgs...>, std::variant<FArgs, SaveArgs...>>;
		using type = typename remove_repeat_helper<std::variant<RArgs...>, inn_type>::type;
	};

	template<typename...SaveArgs>
	struct remove_repeat_helper<std::variant<>, std::variant<SaveArgs...>> {
		using type = std::variant<SaveArgs...>;
	};

	using type = typename remove_repeat_helper<std::variant<Args...>, std::variant<>>::type;
};

template<typename T>
using remove_repeat_t = typename remove_repeat<T>::type;

} //namespace libgs


#endif //LIBGS_CORE_CXX_REMOVE_REPEAT_H
