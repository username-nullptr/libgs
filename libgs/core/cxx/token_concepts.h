
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
#include <libgs/core/cxx/attributes.h>

#ifdef LIBGS_USING_BOOST_ASIO
# include <boost/asio/experimental/awaitable_operators.hpp>
# include <boost/asio/spawn.hpp>
#else
# include <asio/experimental/awaitable_operators.hpp>
#endif //LIBGS_USING_BOOST_ASIO

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

template <typename>
struct is_use_awaitable : std::false_type {};

template <concepts::execution Exec>
struct is_use_awaitable<use_basic_awaitable_t<Exec>> : std::true_type {};

template <typename T>
constexpr bool is_use_awaitable_v = is_use_awaitable<T>::value;

template <concepts::execution Exec>
using basic_redirect_error_t =
	asio::redirect_error_t<use_basic_awaitable_t<Exec>>;

using redirect_error_t =
	basic_redirect_error_t<asio::any_io_executor>;

template <concepts::execution Exec>
using basic_cancellation_slot_binder_t =
	asio::cancellation_slot_binder<use_basic_awaitable_t<Exec>,asio::cancellation_slot>;

using cancellation_slot_binder_t =
	basic_cancellation_slot_binder_t<asio::any_io_executor>;

template <concepts::execution Exec>
using basic_redirect_error_cancellation_slot_binder_t =
	asio::redirect_error_t<asio::cancellation_slot_binder<use_basic_awaitable_t<Exec>,asio::cancellation_slot>>;

using redirect_error_cancellation_slot_binder_t =
	basic_redirect_error_cancellation_slot_binder_t<asio::any_io_executor>;

template <concepts::execution Exec>
using basic_cancellation_slot_binder_redirect_error_t =
	asio::cancellation_slot_binder<asio::redirect_error_t<use_basic_awaitable_t<Exec>>,asio::cancellation_slot>;

using cancellation_slot_binder_redirect_error_t =
	basic_cancellation_slot_binder_redirect_error_t<asio::any_io_executor>;

template <typename>
struct is_redirect_error : std::false_type {};

template <concepts::execution Exec>
struct is_redirect_error <
	basic_redirect_error_t<Exec>
> : std::true_type {};

template <typename T>
constexpr bool is_redirect_error_v =
	is_redirect_error<T>::value;

template <typename>
struct is_cancellation_slot_binder : std::false_type {};

template <concepts::execution Exec>
struct is_cancellation_slot_binder <
	basic_cancellation_slot_binder_t<Exec>
> : std::true_type {};

template <typename T>
constexpr bool is_cancellation_slot_binder_v =
	is_cancellation_slot_binder<T>::value;

template <typename>
struct is_redirect_error_cancellation_slot_binder : std::false_type {};

template <concepts::execution Exec>
struct is_redirect_error_cancellation_slot_binder <
	basic_redirect_error_cancellation_slot_binder_t<Exec>
> : std::true_type {};

template <typename T>
constexpr bool is_redirect_error_cancellation_slot_binder_v =
	is_redirect_error_cancellation_slot_binder<T>::value;

template <typename>
struct is_cancellation_slot_binder_redirect_error : std::false_type {};

template <concepts::execution Exec>
struct is_cancellation_slot_binder_redirect_error <
	basic_cancellation_slot_binder_redirect_error_t<Exec>
> : std::true_type {};

template <typename T>
constexpr bool is_cancellation_slot_binder_redirect_error_v =
	is_cancellation_slot_binder_redirect_error<T>::value;

template <typename T>
struct is_co_opt_token : std::disjunction <
	is_use_awaitable<T>,
	is_redirect_error<T>,
	is_cancellation_slot_binder<T>,
	is_redirect_error_cancellation_slot_binder<T>,
	is_cancellation_slot_binder_redirect_error<T>
> {};

template <typename T>
constexpr bool is_co_opt_token_v = is_co_opt_token<T>::value;

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

template <typename Token>
struct is_dis_func_opt_token
{
	static constexpr bool value =
		is_opt_token_v<Token,asio::error_code> and
		not is_function_v<Token>;
};

template <typename Token>
constexpr bool is_dis_func_opt_token_v = is_dis_func_opt_token<Token>::value;

template <typename Token, typename...Args>
struct is_dis_sync_opt_token
{
	static constexpr bool value =
		is_opt_token_v<Token,Args...> and
		not is_sync_opt_token_v<Token>;
};

template <typename Token, typename...Args>
constexpr bool is_dis_sync_opt_token_v = is_dis_sync_opt_token<Token,Args...>::value;

namespace concepts
{

template <typename T>
concept use_awaitable =
	is_use_awaitable_v<std::remove_cvref_t<T>>;

template <typename T>
concept redirect_error =
	is_redirect_error_v<std::remove_cvref_t<T>>;

template <typename T>
concept cancellation_slot_binder =
	is_cancellation_slot_binder_v<std::remove_cvref_t<T>>;

template <typename T>
concept redirect_error_cancellation_slot_binder =
	is_redirect_error_cancellation_slot_binder_v<std::remove_cvref_t<T>>;

template <typename T>
concept cancellation_slot_binder_redirect_error =
	is_cancellation_slot_binder_redirect_error_v<std::remove_cvref_t<T>>;

template <typename T>
concept co_opt_token = is_co_opt_token_v<std::remove_cvref_t<T>>;

template <typename Token, typename...Args>
concept async_opt_token = is_async_opt_token_v<Token,Args...>;

template <typename Token, typename...Args>
concept sync_opt_token = is_sync_opt_token_v<Token>;

template <typename Token, typename...Args>
concept opt_token = is_opt_token_v<Token,Args...>;

template <typename Token, typename...Args>
concept dis_sync_opt_token = is_dis_sync_opt_token_v<Token,Args...>;

template <typename Token>
concept dis_func_opt_token = is_dis_func_opt_token_v<Token>;

template <typename T>
concept dispatch_token = is_dispatch_token_v<std::remove_cvref_t<T>>;

} //namespace concepts

#ifdef LIBGS_USING_BOOST_ASIO

template <concepts::execution Exec>
using basic_yield_context = asio::basic_yield_context<Exec>;

using yield_context = asio::yield_context;

template <typename>
struct is_basic_yield_context : std::false_type {};

template <concepts::execution Exec>
struct is_basic_yield_context<basic_yield_context<Exec>> : std::true_type {};

template <typename T>
constexpr bool is_basic_yield_context_v = is_basic_yield_context<T>::value;

template <typename T>
constexpr bool is_yield_context_v = is_basic_yield_context_v<yield_context>;

namespace concepts
{

template <typename T>
concept yield_context = is_basic_yield_context_v<T>;

} //namespace concepts

#endif //LIBGS_USING_BOOST_ASIO

namespace operators
{

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    concepts::use_awaitable auto &&ua, std::error_code &error
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    const std::error_code &error, concepts::use_awaitable auto &&ua
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    concepts::use_awaitable auto &&ua, const asio::cancellation_slot &slot
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    const asio::cancellation_slot &slot, libgs::concepts::use_awaitable auto &&ua
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    concepts::redirect_error auto &&re, const asio::cancellation_slot &slot
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    const asio::cancellation_slot &slot, concepts::redirect_error auto &&re
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    concepts::cancellation_slot_binder auto &&csb, std::error_code &error
);

LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    std::error_code &error, concepts::cancellation_slot_binder auto &&csb
);

}} //namespace libgs::operators
#include <libgs/core/cxx/detail/token_concepts.h>


#endif //LIBGS_CORE_CXX_TOKEN_CONCEPTS_H