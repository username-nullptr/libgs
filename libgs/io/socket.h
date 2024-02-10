
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

#ifndef LIBGS_IO_SOCKET_H
#define LIBGS_IO_SOCKET_H

#include <libgs/io/device.h>

namespace libgs::io
{

class socket : public device
{
	LIBGS_DISABLE_COPY_MOVE(socket)

public:
	using address_t = asio::ip::address;
	using port_t = asio::ip::port_type;
	using shutdown_type = asio::socket_base::shutdown_type;

public:
	socket() = default;
	~socket() override;

public:
	virtual void async_connect(const address_t &addr, port_t port, callback_t callback) noexcept = 0;
	[[nodiscard]] virtual error_code connect(const address_t &addr, port_t port) noexcept = 0;
	[[nodiscard]] virtual await_error_t co_connect(const address_t &addr, port_t port) noexcept = 0;

public:
	void async_connect(const std::string &addr, port_t port, callback_t callback) noexcept;
	[[nodiscard]] error_code connect(const std::string &addr, port_t port) noexcept;
	[[nodiscard]] await_error_t co_connect(const std::string &addr, port_t port) noexcept;

public:
	void async_connect(const std::string &domain, callback_t callback) noexcept;
	[[nodiscard]] error_code connect(const std::string &domain) noexcept;
	[[nodiscard]] await_error_t co_connect(const std::string &domain) noexcept;

public:
	[[nodiscard]] virtual address_t remote_address(error_code *error = nullptr) const noexcept = 0;
	[[nodiscard]] virtual port_t remote_port(error_code *error = nullptr) const noexcept = 0;

public:
	[[nodiscard]] virtual address_t local_address(error_code *error = nullptr) const noexcept = 0;
	[[nodiscard]] virtual port_t local_port(error_code *error = nullptr) const noexcept = 0;

public:
	virtual error_code shutdown(shutdown_type what = shutdown_type::shutdown_both) noexcept = 0;
	[[nodiscard]] error_code close(bool shutdown) noexcept;
	using device::close;
};

} //namespace libgs::io


#endif //LIBGS_IO_SOCKET_H
