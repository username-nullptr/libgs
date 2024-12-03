
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

#ifndef LIBGS_CORE_CXX_TOKEN_CONCEPTS_H
#define LIBGS_CORE_CXX_TOKEN_CONCEPTS_H

#include <libgs/core/cxx/asio_concepts.h>

namespace libgs
{

template <concepts::execution Exec = asio::any_io_executor>
using use_basic_awaitable_t = asio::use_awaitable_t<Exec>;

using use_awaitable_t = use_basic_awaitable_t<asio::any_io_executor>;
constexpr auto use_awaitable = asio::use_awaitable;

template <typename Allocator = std::allocator<void>>
using use_basic_future_t = asio::use_future_t<Allocator>;

using use_future_t = use_basic_future_t<std::allocator<void>>;
constexpr auto use_future = asio::use_future;

using detached_t = asio::detached_t;
constexpr auto detached = asio::detached;

struct use_sync_t {};
constexpr use_sync_t use_sync = use_sync_t();

template <typename>
struct is_use_awaitable : std::false_type {};

template <concepts::execution Exec>
struct is_use_awaitable<use_basic_awaitable_t<Exec>> : std::true_type {};

template <typename T>
constexpr bool is_use_awaitable_v = is_use_awaitable<T>::value;

template <typename>
struct is_use_future : std::false_type {};

template <typename Allocator>
struct is_use_future<use_basic_future_t<Allocator>> : std::true_type {};

template <typename T>
constexpr bool is_use_future_v = is_use_future<T>::value;

template <typename>
struct is_detached : std::false_type {};

template <>
struct is_detached<detached_t> : std::true_type {};

template <typename T>
constexpr bool is_detached_v = is_detached<T>::value;

template <typename>
struct is_use_sync : std::false_type {};

template <>
struct is_use_sync<use_sync_t> : std::true_type {};

template <typename T>
constexpr bool is_use_sync_v = is_use_sync<T>::value;

template <typename T>
struct is_dispatch_token : std::disjunction<
	is_use_awaitable<T>, is_use_future<T>, is_detached<T>, is_use_sync<T>
> {};

template <typename T>
constexpr bool is_dispatch_token_v = is_dispatch_token<T>::value;

template <typename Token, typename...Args>
struct is_async_opt_token {
	static constexpr bool value = asio::completion_token_for<Token,void(Args...)>;
};

template <typename Token, typename...Args>
constexpr bool is_async_opt_token_v = is_async_opt_token<Token,Args...>::value;

template <typename Token = use_sync_t>
struct is_sync_opt_token
{
	static constexpr bool value =
		std::is_same_v<Token,asio::error_code&> or
		std::is_same_v<Token,use_sync_t>;
};

template <typename Token = use_sync_t>
constexpr bool is_sync_opt_token_v = is_sync_opt_token<Token>::value;

template <typename Token, typename...Args>
struct is_opt_token
{
	static constexpr bool value =
		is_async_opt_token_v<Token,Args...> or
		is_sync_opt_token_v<Token>;
};

template <typename Token, typename...Args>
constexpr bool is_opt_token_v = is_opt_token<Token,Args...>::value;

template <typename Token = use_sync_t>
struct is_dis_func_opt_token
{
	static constexpr bool value =
		is_opt_token_v<Token,asio::error_code> and
		not is_function_v<Token>;
};

template <typename Token>
constexpr bool is_dis_func_opt_token_v = is_dis_func_opt_token<Token>::value;

namespace concepts
{

template <typename Token, typename...Args>
concept async_opt_token = is_async_opt_token_v<Token,Args...>;

template <typename Token, typename...Args>
concept sync_opt_token = is_sync_opt_token_v<Token>;

template <typename Token, typename...Args>
concept opt_token = is_opt_token_v<Token,Args...>;

template <typename Token>
concept dis_func_opt_token = is_dis_func_opt_token_v<Token>;

template <typename T>
concept dispatch_token = is_dispatch_token_v<std::remove_cvref_t<T>>;

}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_TOKEN_CONCEPTS_H