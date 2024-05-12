
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

#ifndef LIBGS_IO_SSL_TCP_SOCKET
#define LIBGS_IO_SSL_TCP_SOCKET

// TODO ...

#include <libgs/io/cxx/option_ssl>
//#ifdef LIBGS_DISABLE_SSL
#if 1 //tmp

#include <libgs/io/tcp_socket.h>
#include <libgs/io/detail/ssl_tcp_socket_data.h>

namespace libgs::io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_CORE_TAPI basic_ssl_tcp_socket : public basic_tcp_socket<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_ssl_tcp_socket)
	using base_type = basic_socket<Exec>;

public:
	using executor_type = base_type::executor_type;
	using address_vector = base_type::address_vector;
	using shutdown_type = base_type::shutdown_type;

	using asio_socket_type = base_type::asio_socket_type;
	using asio_socket_ptr = base_type::asio_socket_ptr;
	using resolver = base_type::resolver;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_ssl_tcp_socket(Context &context = execution::io_context());

	template <concept_execution Exec0>
	basic_ssl_tcp_socket(asio_basic_tcp_socket<Exec0> &&sock);

	explicit basic_ssl_tcp_socket(const executor_type &exec);

	[[nodiscard]] asio_socket_type &native_object() override;

protected:
	[[nodiscard]] awaitable<error_code> do_connect
	(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept override;

	[[nodiscard]] awaitable<address_vector> do_dns
	(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept override;

	[[nodiscard]] awaitable<size_t> read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept override;

	[[nodiscard]] awaitable<size_t> write_data
	(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept override;
};

using ssl_tcp_socket = basic_ssl_tcp_socket<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_ssl_tcp_socket_ptr = std::shared_ptr<basic_ssl_tcp_socket<Exec>>;

using ssl_tcp_socket_ptr = basic_ssl_tcp_socket_ptr<>;

template <concept_execution Exec, typename...Args>
basic_ssl_tcp_socket_ptr<Exec> make_basic_ssl_tcp_socket(Args&&...args);

template <typename...Args>
ssl_tcp_socket_ptr make_ssl_tcp_socket(Args&&...args);

} //namespace libgs::io
#include <libgs/io/detail/ssl_tcp_socket.h>

#endif //LIBGS_DISABLE_SSL

#endif //LIBGS_IO_SSL_TCP_SOCKET
