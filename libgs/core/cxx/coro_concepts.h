
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

#ifndef LIBGS_CORE_CXX_CORO_CONCEPTS_H
#define LIBGS_CORE_CXX_CORO_CONCEPTS_H

#include <libgs/core/cxx/concepts.h>

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
struct is_use_awaitable : std::false_type {};

template <concepts::execution Exec>
struct is_use_awaitable <
	use_basic_awaitable_t<Exec>
> : std::true_type {};

template <typename T>
constexpr bool is_use_awaitable_v =
	is_use_awaitable<T>::value;

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
constexpr bool is_co_token_v =
	is_use_awaitable_v<T> or
	is_redirect_error_v<T> or
	is_cancellation_slot_binder_v<T> or
	is_redirect_error_cancellation_slot_binder_v<T> or
	is_cancellation_slot_binder_redirect_error_v<T>;

namespace concepts
{

template <typename T>
concept use_awaitable =
	not std::is_pointer_v<T> and is_use_awaitable_v<std::decay_t<T>>;

template <typename T>
concept redirect_error =
	not std::is_pointer_v<T> and is_redirect_error_v<std::decay_t<T>>;

template <typename T>
concept cancellation_slot_binder =
	not std::is_pointer_v<T> and is_cancellation_slot_binder_v<std::decay_t<T>>;

template <typename T>
concept redirect_error_cancellation_slot_binder =
	not std::is_pointer_v<T> and is_redirect_error_cancellation_slot_binder_v<std::decay_t<T>>;

template <typename T>
concept cancellation_slot_binder_redirect_error =
	not std::is_pointer_v<T> and is_cancellation_slot_binder_redirect_error_v<std::decay_t<T>>;

template <typename T>
concept co_token =
	not std::is_pointer_v<T> and is_co_token_v<std::decay_t<T>>;

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

template <concepts::use_awaitable Token>
[[nodiscard]] auto operator|(Token &&ua, std::error_code &error) {
	return asio::redirect_error(std::forward<Token>(ua), error);
}

template <concepts::use_awaitable Token>
[[nodiscard]] auto operator|(const std::error_code &error, Token &&ua) {
	return asio::redirect_error(std::forward<Token>(ua), error);
}

template <concepts::use_awaitable Token>
[[nodiscard]] auto operator|(Token &&ua, const asio::cancellation_slot &slot) {
	return asio::bind_cancellation_slot(slot, std::forward<Token>(ua));
}

template <libgs::concepts::use_awaitable Token>
[[nodiscard]] auto operator|(const asio::cancellation_slot &slot, Token &&ua) {
	return asio::bind_cancellation_slot(slot, std::forward<Token>(ua));
}

template <concepts::redirect_error Token>
[[nodiscard]] auto operator|(Token &&re, const asio::cancellation_slot &slot) {
	return asio::bind_cancellation_slot(slot, std::forward<Token>(re));
}

template <concepts::redirect_error Token>
[[nodiscard]] auto operator|(const asio::cancellation_slot &slot, Token &&re) {
	return asio::bind_cancellation_slot(slot, std::forward<Token>(re));
}

template <concepts::cancellation_slot_binder Token>
[[nodiscard]] auto operator|(Token &&csb, std::error_code &error) {
	return asio::redirect_error(std::forward<Token>(csb), error);
}

template <concepts::cancellation_slot_binder Token>
[[nodiscard]] auto operator|(std::error_code &error, Token &&csb) {
	return asio::redirect_error(std::forward<Token>(csb), error);
}

}} // namespace libgs::operators


#endif //LIBGS_CORE_CXX_CORO_CONCEPTS_H
