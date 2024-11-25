
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

#ifndef LIBGS_HTTP_OPT_TOKEN_H
#define LIBGS_HTTP_OPT_TOKEN_H

#include <libgs/http/cxx/file_opt_token.h>
#include <libgs/http/types.h>

namespace libgs::http
{

template <typename...Args>
using callback_t = std::function<void(Args...)>;

template <typename Token, typename...Args>
struct LIBGS_HTTP_TAPI is_async_token {
	static constexpr bool value = asio::completion_token_for<Token,void(Args...)>;
};

template <typename Token, typename...Args>
constexpr bool is_async_token_v = is_async_token<Token,Args...>::value;

struct LIBGS_HTTP_VAPI use_sync_type {};
constexpr use_sync_type use_sync = use_sync_type();

template <typename Token = use_sync_type>
struct LIBGS_HTTP_TAPI is_sync_token {
	static constexpr bool value =
		std::is_same_v<Token,error_code&> or
		std::is_same_v<std::remove_cvref_t<Token>,use_sync_type>;
};

template <typename Token = use_sync_type>
constexpr bool is_sync_token_v = is_sync_token<Token>::value;

template <typename Token, typename...Args>
struct LIBGS_HTTP_TAPI is_token {
	static constexpr bool value = is_async_token_v<Token,Args...> or is_sync_token_v<Token>;
};

template <typename Token, typename...Args>
constexpr bool is_token_v = is_token<Token, Args...>::value;

template <typename Token = use_sync_type>
struct LIBGS_HTTP_TAPI is_dis_func_token {
	static constexpr bool value = is_token_v<Token,error_code> and not is_function_v<Token>;
};

template <typename Token>
constexpr bool is_dis_func_token_v = is_dis_func_token<Token>::value;

namespace concepts
{

template <typename Token, typename...Args>
concept async_token = is_async_token_v<Token,Args...>;

template <typename Token, typename...Args>
concept sync_token = is_sync_token_v<Token>;

template <typename Token, typename...Args>
concept token = is_token_v<Token,Args...>;

template <typename Token>
concept dis_func_token = is_dis_func_token_v<Token>;

}} //namespace libgs::http::concepts


#endif //LIBGS_HTTP_OPT_TOKEN_H
