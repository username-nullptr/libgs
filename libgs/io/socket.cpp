
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

#include "socket.h"
#include <iostream>

namespace libgs::io
{

socket::~socket() = default;

void socket::async_connect(const std::string &addr, port_t port, callback_t callback) noexcept
{
	async_connect(address_t::from_string(addr), port, std::move(callback));
}

error_code socket::connect(const std::string &addr, port_t port) noexcept
{
	return connect(address_t::from_string(addr), port);
}

socket::await_error_t socket::co_connect(const std::string &addr, port_t port) noexcept
{
	co_return co_await co_connect(address_t::from_string(addr), port);
}

void socket::async_connect(const std::string &domain, callback_t callback) noexcept
{
	// TODO ...
}

error_code socket::connect(const std::string &domain) noexcept
{
	// TODO ...
	return {};
}

socket::await_error_t socket::co_connect(const std::string &domain) noexcept
{
	// TODO ...
	co_return error_code();
}

error_code socket::close(bool shutdown) noexcept
{
	if( not shutdown )
		return close();

	auto error = close();
	if( error )
		return error;
	return this->shutdown();
}

} //namespace libgs::io
