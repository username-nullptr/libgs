
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

#ifndef LIBGS_HTTP_BASIC_OPT_TOKEN_H
#define LIBGS_HTTP_BASIC_OPT_TOKEN_H

#include <libgs/http/basic/types.h>

#ifdef LIBGS_ENABLE_OPENSSL
#include <asio/ssl.hpp>
#endif //LIBGS_ENABLE_OPENSSL

namespace libgs::http
{

template <typename Stream>
struct stream_requires : std::false_type {};

template <concept_execution Exec>
struct stream_requires<asio::basic_stream_socket<asio::ip::tcp,Exec>> : std::true_type {};

#ifdef LIBGS_ENABLE_OPENSSL
template <concept_execution Exec>
struct stream_requires<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> : std::true_type {};
#endif //LIBGS_ENABLE_OPENSSL

template <typename Stream>
constexpr bool stream_requires_v = stream_requires<Stream>::value;

template <typename Stream>
concept concept_stream_requires = stream_requires_v<Stream>;

using begin_t = size_t;
using total_t = size_t;
using end_t   = size_t;

template <typename...Args>
using callback_t = std::function<void(Args...)>;

struct req_range
{
	size_t begin = 0;
	size_t total = 0;

	constexpr req_range();
	req_range(size_t total);
	req_range(size_t begin, size_t total);
};

struct resp_range
{
	size_t begin = 0;
	size_t end = 0;

	constexpr resp_range();
	resp_range(size_t end);
	resp_range(size_t begin, size_t end);
};
using resp_ranges = std::vector<resp_range>;

} //namespace libgs::http
#include <libgs/http/basic/detail/opt_token.h>


#endif //LIBGS_HTTP_BASIC_OPT_TOKEN_H
