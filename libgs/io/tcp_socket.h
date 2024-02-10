
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

#ifndef LIBGS_IO_TCP_SOCKET_H
#define LIBGS_IO_TCP_SOCKET_H

#include <libgs/io/socket.h>

namespace libgs
{
	
using asio_tcp_socket = asio::ip::tcp::socket;
using asio_tcp_socket_ptr = std::shared_ptr<asio_tcp_socket>;

namespace io
{

class tcp_socket : public socket
{
	LIBGS_DISABLE_COPY_MOVE(tcp_socket)

public:
	explicit tcp_socket(asio::io_context &io = io_context());
	explicit tcp_socket(asio_tcp_socket &&sock);
	~tcp_socket() override;

public:
	void async_connect(const address_t &addr, port_t port, callback_t callback) noexcept override;
	[[nodiscard]] error_code connect(const address_t &addr, port_t port) noexcept override;
	[[nodiscard]] await_error_t co_connect(const address_t &addr, port_t port) noexcept override;

public:
	using socket::async_connect;
	using socket::connect;
	using socket::co_connect;

public:
	void async_read_some(void *buf, size_t size, rw_callback_t callback) noexcept override;
	[[nodiscard]] size_t read_some(void *buf, size_t size, error_code *error = nullptr) noexcept override;
	[[nodiscard]] await_size_t co_read_some(void *buf, size_t size, error_code *error = nullptr) noexcept override;

public:
	using socket::async_read_some;
	using socket::read_some;
	using socket::co_read_some;

public:
	void async_write_some(const void *buf, size_t size, rw_callback_t callback) noexcept override;
	size_t write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept override;
	[[nodiscard]] await_size_t co_write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept override;

public:
	using socket::async_write_some;
	using socket::write_some;
	using socket::co_write_some;

public:
	[[nodiscard]] address_t remote_address(error_code *error = nullptr) const noexcept override;
	[[nodiscard]] port_t remote_port(error_code *error = nullptr) const noexcept override;

public:
	[[nodiscard]] address_t local_address(error_code *error = nullptr) const noexcept override;
	[[nodiscard]] port_t local_port(error_code *error = nullptr) const noexcept override;

public:
	error_code shutdown(shutdown_type what = shutdown_type::shutdown_both) noexcept override;
	[[nodiscard]] virtual error_code close() noexcept override;
	[[nodiscard]] virtual bool is_open() const noexcept override;

public:
	bool wait_writeable(const duration &ms, error_code *error = nullptr) noexcept override;
	bool wait_readable(const duration &ms, error_code *error = nullptr) noexcept override;

public:
	template <typename SettableSocketOption>
	error_code set_option(const SettableSocketOption &option) noexcept;

	template <typename GettableSocketOption>
	error_code get_option(GettableSocketOption &option) const noexcept;

protected:
	asio_tcp_socket_ptr m_sock;
};

using tcp_socket_ptr = std::shared_ptr<tcp_socket>;

template <typename...Args>
tcp_socket_ptr make_tcp_socket(Args&&...args);

}} //namespace libgs::io
#include <libgs/io/detail/tcp_socket.h>


#endif //LIBGS_IO_TCP_SOCKET_H
