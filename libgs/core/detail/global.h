
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
void sleep_for(const duration<Rep,Period> &rtime)
{
	std::this_thread::sleep_for(rtime);
}

template<typename Clock, typename Duration>
void sleep_until(const time_point<Clock,Duration> &atime)
{
	std::this_thread::sleep_until(atime);
}

template<typename Rep, typename Period>
decltype(auto) get_associated_redirect_time
(concepts::any_async_tf_opt_token auto &&token, const duration<Rep,Period> &def_time)
{
	using Token = decltype(token);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_redirect_time_v<token_t> )
		return return_reference(token.time);
	else
		return std::chrono::duration_cast<milliseconds>(def_time);
}

decltype(auto) get_associated_redirect_time(concepts::any_async_tf_opt_token auto &&token)
{
	using token_t = decltype(token);
	return get_associated_redirect_time(std::forward<token_t>(token), milliseconds(0));
}

constexpr decltype(auto) unbound_redirect_time(concepts::any_async_tf_opt_token auto &&token)
{
	using Token = decltype(token);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_redirect_time_v<token_t> )
		return return_reference(std::forward<Token>(token).token);
	else
		return std::forward<Token>(token);
}

namespace operators
{

template <concepts::any_async_tf_opt_token Token>
auto operator|(Token &&token, error_code &error)
	requires (not is_redirect_error_v<std::remove_cvref_t<Token>>)
{
	if constexpr( is_redirect_time_v<Token> )
	{
		auto _token = asio::redirect_error(token.token, error);
		using token_t = std::remove_cvref_t<decltype(_token)>;
		return redirect_time_t<token_t>(std::move(_token), token.time);
	}
	else
		return asio::redirect_error(std::forward<Token>(token), error);
}

template <concepts::any_async_tf_opt_token Token>
auto operator|(Token &&token, const asio::cancellation_slot &slot)
	requires (not is_cancellation_slot_binder_v<std::remove_cvref_t<Token>>)
{
	if constexpr( is_redirect_time_v<Token> )
	{
		auto _token = asio::bind_cancellation_slot(slot, token.token);
		using token_t = std::remove_cvref_t<decltype(_token)>;
		return redirect_time_t<token_t>(std::move(_token), token.time);
	}
	else
		return asio::bind_cancellation_slot(slot, std::forward<Token>(token));
}

template <typename Rep, typename Period>
auto operator|(concepts::any_async_opt_token auto &&token, const duration<Rep,Period> &d)
{
	using token_t = decltype(token);
	return redirect_time(std::forward<token_t>(token), d);
}

}} //namespace libgs::operators


#endif //LIBGS_CORE_DETAIL_GLOBAL_H