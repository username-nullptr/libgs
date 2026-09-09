// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_OPT_TOKEN_H
#define LIBGS_CORE_UTILS_OPT_TOKEN_H

#include <libgs/core/utils/asio_concepts.h>
#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>

#ifdef LIBGS_USING_BOOST_ASIO
# include <boost/asio/experimental/awaitable_operators.hpp>
# include <boost/asio/spawn.hpp>
#else
# include <asio/experimental/awaitable_operators.hpp>
#endif //LIBGS_USING_BOOST_ASIO

namespace libgs
{

template <concepts::exec Exec = asio::any_io_executor>
using use_basic_awaitable_t = asio::use_awaitable_t<Exec>;

using use_awaitable_t = use_basic_awaitable_t<asio::any_io_executor>;
constexpr auto use_awaitable = asio::use_awaitable;

template <typename Allocator = std::allocator<void>>
using use_basic_future_t = asio::use_future_t<Allocator>;

using use_future_t = use_basic_future_t<std::allocator<void>>;
constexpr auto use_future = asio::use_future;

using detached_t = asio::detached_t;
constexpr auto detached = asio::detached;

using deferred_t = asio::deferred_t;
constexpr auto deferred = asio::deferred;

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
	redirect_time_t(auto &&completion_token, const duration<Rep,Period> &rtime);

	template <typename Clock, typename Duration>
	redirect_time_t(auto &&completion_token, const time_point<Clock,Duration> &atime);

	token_t token;
	milliseconds time {0};
};

template <typename Token, typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI auto redirect_time (
	Token &&token, const duration<Rep,Period> &timeout
);

template <typename Token, typename Clock, typename Duration>
[[nodiscard]] LIBGS_CORE_TAPI auto redirect_time (
	Token &&token, const time_point<Clock,Duration> &timeout
);

} //namespace libgs
#include <libgs/core/utils/detail/opt_token.h>


#endif //LIBGS_CORE_UTILS_OPT_TOKEN_H
