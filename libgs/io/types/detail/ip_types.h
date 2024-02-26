
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

#ifndef LIBGS_IO_TYPES_DETAIL_IP_TYPES_H
#define LIBGS_IO_TYPES_DETAIL_IP_TYPES_H

namespace libgs::io
{

host_endpoint::host_endpoint(string_wrapper host, uint16_t port)
{
	this->host = std::move(host);
	this->port = port;
}

ip_endpoint::ip_endpoint(ip_address addr, uint16_t port) :
	addr(std::move(addr)), port(port)
{

}

ip_endpoint::ip_endpoint(concept_ip_endpoint auto &&ep) :
	ip_endpoint(ep.address(), ep.port())
{

}

socket_option::socket_option(auto &data) :
	id(typeid(data).hash_code()), data(&data) 
{

}

} //namespace libgs::io


#endif //LIBGS_IO_TYPES_DETAIL_IP_TYPES_H
