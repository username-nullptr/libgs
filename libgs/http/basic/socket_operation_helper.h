
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

#ifndef LIBGS_HTTP_BASIC_SOCKET_OPERATION_HELPER_H
#define LIBGS_HTTP_BASIC_SOCKET_OPERATION_HELPER_H

#include <libgs/http/global.h>

namespace libgs::http
{

template <typename Stream>
class socket_operation_helper;

template <concept_execution Exec>
class LIBGS_HTTP_TAPI socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>
{
	using socket_t = asio::basic_stream_socket<asio::ip::tcp,Exec>;
	using executor_t = typename socket_t::executor_type;
	using endpoint_t = asio::ip::tcp::endpoint;

public:
	static void get_option(socket_t &socket, auto &option, error_code &error);
	static void close(socket_t &socket);

	static endpoint_t remote_endpoint(socket_t &socket);
	static endpoint_t local_endpoint(socket_t &socket);

	static const executor_t &get_executor(socket_t &socket) noexcept;
	static bool is_open(socket_t &socket) noexcept;
};

#ifdef LIBGS_ENABLE_OPENSSL

template <concept_execution Exec>
class LIBGS_HTTP_TAPI socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>
{
	using socket_t = asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>;
	using executor_t = typename asio::basic_stream_socket<asio::ip::tcp,Exec>::executor_type;
	using endpoint_t = asio::ip::tcp::endpoint;

public:
	static void get_option(socket_t &socket, auto &option, error_code &error);
	static void close(socket_t &socket);

	static endpoint_t remote_endpoint(socket_t &socket);
	static endpoint_t local_endpoint(socket_t &socket);

	static const executor_t &get_executor(socket_t &socket) noexcept;
	static bool is_open(socket_t &socket) noexcept;
};

#endif //LIBGS_ENABLE_OPENSSL

} //namespace libgs::http
#include <libgs/http/basic/detail/socket_operation_helper.h>


#endif //LIBGS_HTTP_BASIC_SOCKET_OPERATION_HELPER_H
