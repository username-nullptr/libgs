
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

#include "tcp_socket.h"

namespace libgs::io
{

tcp_socket::tcp_socket()
{

}

tcp_socket::tcp_socket(asio_tcp_socket &&sock)
{

}

tcp_socket::~tcp_socket() 
{

}

void tcp_socket::async_connect(const address_t &addr, port_t port, callback_t callback) noexcept
{

}

error_code tcp_socket::connect(const address_t &addr, port_t port) noexcept 
{

}

tcp_socket::await_error_t tcp_socket::co_connect(const address_t &addr, port_t port) noexcept
{

}

void tcp_socket::async_read_some(void *buf, size_t size, rw_callback_t callback) noexcept 
{

}

size_t tcp_socket::read_some(void *buf, size_t size, error_code *error = nullptr) noexcept 
{

}

tcp_socket::await_size_t tcp_socket::co_read_some(void *buf, size_t size, error_code *error = nullptr) noexcept
{

}

void tcp_socket::async_write_some(const void *buf, size_t size, rw_callback_t callback) noexcept 
{

}

size_t tcp_socket::write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept 
{

}

tcp_socket::await_size_t tcp_socket::co_write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept
{

}

tcp_socket::address_t tcp_socket::remote_address(error_code *error = nullptr) noexcept
{

}

tcp_socket::port_t tcp_socket::remote_port(error_code *error = nullptr) noexcept
{

}

tcp_socket::address_t tcp_socket::local_address(error_code *error = nullptr) noexcept
{

}

tcp_socket::port_t tcp_socket::local_port(error_code *error = nullptr) noexcept
{

}

error_code tcp_socket::shutdown(shutdown_type what = shutdown_type::shutdown_both) noexcept 
{

}

} //namespace libgs::io
