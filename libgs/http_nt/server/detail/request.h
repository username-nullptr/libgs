
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_request<Connection>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl(connection_t &&connection, parser_t &&parser) :
		m_connection(std::move(connection)),
		m_parser(std::move(parser))
	{

	}

public:


public:
	connection_t m_connection;
	parser_t m_parser {};
};

template <concepts::connection Connection>
basic_request<Connection>::basic_request(connection_t &&connection, parser_t &&parser) :
	const_headers<basic_request>(nullptr),
	const_cookies<value_t,basic_request>(nullptr),
	const_parameters<basic_request>(nullptr),
	m_impl(new impl(std::move(connection), std::move(parser)))
{

}

template <concepts::connection Connection>
basic_request<Connection>::~basic_request()
{
	delete m_impl;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H