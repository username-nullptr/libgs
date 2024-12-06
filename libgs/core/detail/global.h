
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

#ifndef LIBGS_CORE_DETAIL_GLOBAL_H
#define LIBGS_CORE_DETAIL_GLOBAL_H

namespace libgs
{

template<typename Rep, typename Period>
void sleep_for(const std::chrono::duration<Rep,Period> &rtime)
{
	std::this_thread::sleep_for(rtime);
}

template<typename Clock, typename Duration>
void sleep_until(const std::chrono::time_point<Clock,Duration> &atime)
{
	std::this_thread::sleep_until(atime);
}

auto co_opt_token_helper(concepts::co_opt_token auto &&token)
{
	using token_t = std::remove_cvref_t<decltype(token)>;
	if constexpr( is_redirect_time_v<token_t> )
		return token.token;
	else
		return token;
}

namespace operators
{

auto operator|(concepts::use_awaitable auto &&ua, error_code &error)
{
	using token_t = decltype(ua);
	return asio::redirect_error(std::forward<token_t>(ua), error);
}

auto operator|(concepts::use_awaitable auto &&ua, const asio::cancellation_slot &slot)
{
	using token_t = decltype(ua);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(ua));
}

auto operator|(concepts::redirect_error auto &&re, const asio::cancellation_slot &slot)
{
	using token_t = decltype(re);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(re));
}

auto operator|(concepts::cancellation_slot_binder auto &&csb, error_code &error)
{
	using token_t = decltype(csb);
	return asio::redirect_error(std::forward<token_t>(csb), error);
}

template <typename Token, typename Rep, typename Period>
auto operator|(Token &&token, const duration<Rep,Period> &d) requires
    concepts::co_opt_token<Token> and (not is_redirect_time_v<Token>)
{
	using token_t = std::remove_cvref_t<decltype(token)>;
	return redirect_time_t<token_t>(std::forward<Token>(token), d);
}

auto operator|(concepts::redirect_time auto &&rt, error_code &error)
{
	auto _token = rt.token | error;
	using token_t = std::remove_cvref_t<decltype(_token)>;
	return redirect_time_t<token_t>(std::move(_token), rt.time);
}

auto operator|(concepts::redirect_time auto &&rt, const asio::cancellation_slot &slot)
{
	auto _token = rt.token | slot;
	using token_t = std::remove_cvref_t<decltype(_token)>;
	return redirect_time_t<token_t>(std::move(_token), rt.time);
}

}} //namespace libgs::operators


#endif //LIBGS_CORE_DETAIL_GLOBAL_H