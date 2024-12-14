
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

#ifndef LIBGS_CORE_CXX_OPT_TOKEN_H
#define LIBGS_CORE_CXX_OPT_TOKEN_H

#include <libgs/core/cxx/type_traits.h>

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
constexpr use_sync_t use_sync;

template <typename Token>
using redirect_error_t = asio::redirect_error_t<Token>;

template <typename Token, typename CancellationSlot>
using cancellation_slot_binder = asio::cancellation_slot_binder<Token, CancellationSlot>;

template <typename Token>
class LIBGS_CORE_TAPI redirect_time_t
{
public:
	using token_t = Token;

	template <typename Rep, typename Period>
	redirect_time_t(auto &&token, const duration<Rep,Period> &timeout);

	token_t token;
	milliseconds time {0};
};

template <typename Token, typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI auto redirect_time (
	Token &&token, const duration<Rep,Period> &timeout
);

} //namespace libgs
#include <libgs/core/cxx/detail/opt_token.h>


#endif //LIBGS_CORE_CXX_OPT_TOKEN_H