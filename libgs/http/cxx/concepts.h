
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

#ifndef LIBGS_HTTP_CXX_CONCEPTS_H
#define LIBGS_HTTP_CXX_CONCEPTS_H

#include <libgs/core/global.h>

#ifdef LIBGS_ENABLE_OPENSSL
#include <asio/ssl.hpp>
#endif //LIBGS_ENABLE_OPENSSL

namespace libgs::http
{

template <typename>
struct is_stream : std::false_type {};

template <concepts::execution Exec>
struct is_stream<asio::basic_stream_socket<asio::ip::tcp,Exec>> : std::true_type {};

#ifdef LIBGS_ENABLE_OPENSSL
template <concepts::execution Exec>
struct is_stream<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> : std::true_type {};
#endif //LIBGS_ENABLE_OPENSSL

template <typename Stream>
constexpr bool is_stream_v = is_stream<Stream>::value;

template <typename Stream>
struct is_any_exec_stream
{
	static constexpr bool value = is_stream_v<Stream> and
		std::is_same_v<typename Stream::executor_type, asio::any_io_executor>;
};

template <typename Stream>
constexpr bool is_any_exec_stream_v = is_any_exec_stream<Stream>::value;

namespace concepts
{

template <typename Stream>
concept stream = is_stream_v<Stream>;

template <typename Stream>
concept any_exec_stream = is_any_exec_stream_v<Stream>;

template <typename Token, typename...Signatures>
concept ioop_token =
	asio::completion_token_for<Token,Signatures...> or
	(
		std::is_same_v<std::decay_t<Token>, error_code> and
		std::is_lvalue_reference_v<Token>
	);

} //namespace concepts

namespace core_concepts = libgs::concepts;

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_CONCEPTS_H
