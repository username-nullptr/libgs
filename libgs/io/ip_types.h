
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

#ifndef LIBGS_IO_IP_TYPES_H
#define LIBGS_IO_IP_TYPES_H

#include <libgs/io/global.h>

namespace libgs::io
{

using ip_address = asio::ip::address;

struct host_endpoint
{
	std::string host;
	uint16_t port;

	host_endpoint(concept_string_type auto host, uint16_t port)
	{
		if constexpr( is_char_string_v<decltype(host)> )
			this->host = std::move(host);
		else
			this->host = wcstombs(host);
		this->port = port;
	}
};

template <typename Endpoint>
concept concept_ip_endpoint = requires(Endpoint &&ep) 
{
	std::is_same_v<decltype(ep.address().to_string()), std::string>;
	std::is_arithmetic_v<decltype(ep.port())>;
	sizeof(decltype(ep.port())) == 2;
};

struct ip_endpoint
{
	ip_address addr;
	uint16_t port;

	ip_endpoint(ip_address addr, uint16_t port) :
		addr(std::move(addr)), port(port) {}

	ip_endpoint(concept_ip_endpoint auto &&ep) :
		ip_endpoint(ep.address(), ep.port()) {}
};

struct socket_option
{
	std_type_id id;
	void *data;

	socket_option(auto &data) :
		id(typeid(data).hash_code()), data(&data) {}
};

} //namespace libgs::io


#endif //LIBGS_IO_IP_TYPES_H
