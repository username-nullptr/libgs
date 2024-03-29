
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_H
#define LIBGS_HTTP_SERVER_REQUEST_H

#include <libgs/http/server/datagram.h>
#include <libgs/io/socket.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_server_request
{
	LIBGS_DISABLE_COPY(basic_server_request)

public:
	using datagram_type = basic_server_datagram<CharT>;

public:
	basic_server_request(io::socket_ptr socket, const datagram_type &datagram);
	basic_server_request(io::socket_ptr socket, datagram_type &&datagram);

	basic_server_request(basic_server_request&&) noexcept = default;
	basic_server_request &operator=(basic_server_request&&) noexcept = default;

public:
	// TODO ...

protected:
	io::socket_ptr m_socket;
	datagram_type m_datagram;
};

using server_request = basic_server_request<char>;
using server_wrequest = basic_server_request<wchar_t>;

} //namespace libgs::http

namespace libgs::http
{

template <concept_char_type CharT>
basic_server_request<CharT>::basic_server_request(io::socket_ptr socket, const datagram_type &datagram) :
	m_socket(std::move(socket)), m_datagram(datagram)
{

}

template <concept_char_type CharT>
basic_server_request<CharT>::basic_server_request(io::socket_ptr socket, datagram_type &&datagram) :
	m_socket(std::move(socket)), m_datagram(std::move(datagram))
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_REQUEST_H
