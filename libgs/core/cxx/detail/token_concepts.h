
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

#ifndef LIBGS_CORE_CXX_DETAIL_TOKEN_CONCEPTS_H
#define LIBGS_CORE_CXX_DETAIL_TOKEN_CONCEPTS_H

namespace libgs::operators
{

auto operator|(concepts::use_awaitable auto &&ua, std::error_code &error)
{
	using token_t = decltype(ua);
	return asio::redirect_error(std::forward<token_t>(ua), error);
}

auto operator|(const std::error_code &error, concepts::use_awaitable auto &&ua)
{
	using token_t = decltype(ua);
	return asio::redirect_error(std::forward<token_t>(ua), error);
}

auto operator|(concepts::use_awaitable auto &&ua, const asio::cancellation_slot &slot)
{
	using token_t = decltype(ua);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(ua));
}

auto operator|(const asio::cancellation_slot &slot, concepts::use_awaitable auto &&ua)
{
	using token_t = decltype(ua);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(ua));
}

auto operator|(concepts::redirect_error auto &&re, const asio::cancellation_slot &slot)
{
	using token_t = decltype(re);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(re));
}

auto operator|(const asio::cancellation_slot &slot, concepts::redirect_error auto &&re)
{
	using token_t = decltype(re);
	return asio::bind_cancellation_slot(slot, std::forward<token_t>(re));
}

auto operator|(concepts::cancellation_slot_binder auto &&csb, std::error_code &error)
{
	using token_t = decltype(csb);
	return asio::redirect_error(std::forward<token_t>(csb), error);
}

auto operator|(std::error_code &error, concepts::cancellation_slot_binder auto &&csb)
{
	using token_t = decltype(csb);
	return asio::redirect_error(std::forward<token_t>(csb), error);
}

} //namespace libgs::operators


#endif //LIBGS_CORE_CXX_DETAIL_TOKEN_CONCEPTS_H