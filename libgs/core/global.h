
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

#ifndef LIBGS_CORE_GLOBAL_H
#define LIBGS_CORE_GLOBAL_H

#include <libgs/core/cxx/formatter.h>
#include <libgs/core/cxx/exception.h>
#include <libgs/core/cxx/operators.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_API const char *version_string();

[[nodiscard]] LIBGS_CORE_API const char *text_code();

LIBGS_CORE_API std::thread::id this_thread_id();

[[noreturn]] LIBGS_CORE_API void forced_termination();

template<typename Rep, typename Period>
[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) get_associated_redirect_time (
    concepts::any_async_tf_opt_token auto &&token, const duration<Rep,Period> &def_time
);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) get_associated_redirect_time (
    concepts::any_async_tf_opt_token auto &&token
);

[[nodiscard]] constexpr decltype(auto) unbound_redirect_time (
    concepts::any_async_tf_opt_token auto &&token
);

namespace operators
{

template <concepts::any_async_tf_opt_token Token>
LIBGS_CORE_TAPI [[nodiscard]] auto operator|(Token &&token, error_code &error)
    requires (not is_redirect_error_v<std::remove_cvref_t<Token>>);

template <concepts::any_async_tf_opt_token Token>
LIBGS_CORE_TAPI [[nodiscard]] auto operator|(Token &&token, const asio::cancellation_slot &slot)
    requires (not is_cancellation_slot_binder_v<std::remove_cvref_t<Token>>);

template <typename Rep, typename Period>
LIBGS_CORE_TAPI [[nodiscard]] auto operator| (
    concepts::any_async_opt_token auto &&token, const duration<Rep,Period> &d
);

}} //namespace libgs
#include <libgs/core/detail/global.h>


#endif //LIBGS_CORE_GLOBAL_H
