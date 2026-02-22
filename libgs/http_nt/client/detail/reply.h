
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_reply<Connection>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(connection_t *connection) :
		m_connection(&connection) {}

public:

public:
	connection_t *m_connection = nullptr;
	parser_t m_parser {};
};

template <concepts::connection Connection>
basic_reply<Connection>::basic_reply(connection_t &connection) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(new impl(&connection))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
}

template <concepts::connection Connection>
basic_reply<Connection>::~basic_reply() = default;

template <concepts::connection Connection>
basic_reply<Connection>::basic_reply(basic_reply &&other) noexcept :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::move(other.m_impl))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();

	other.m_impl = std::make_shared<impl>(nullptr);
	other.m_headers = &other.m_impl->m_parser.headers();
	other.m_cookies = &other.m_impl->m_parser.cookies();
}

template <concepts::connection Connection>
basic_reply<Connection> &basic_reply<Connection>::operator=(basic_reply &&other) noexcept
{
	if( other.m_impl != this )
		return *this;

	m_impl = std::move(other.m_impl);
	other.m_impl = std::make_shared<impl>(nullptr);

	other.m_headers = &other.m_impl->m_parser.headers();
	other.m_cookies = &other.m_impl->m_parser.cookies();
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H