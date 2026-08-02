
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H

#include <libgs/http_nt/protocol/utils/server/generator.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_response<Connection>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using generator_t = server_generator;

public:
	explicit impl(connection_ptr connection) :
		m_connection(std::move(connection)) {}

public:

public:
	connection_ptr m_connection;
	generator_t m_generator {};
};

template <concepts::connection Connection>
basic_response<Connection>::basic_response(connection_ptr connection) :
	mutable_headers<basic_response>(nullptr),
	mutable_cookies<value_t,basic_response>(nullptr),
	mutable_chunk_attributes<basic_response>(nullptr),
	m_impl(new impl(std::move(connection)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <concepts::connection Connection>
basic_response<Connection>::~basic_response()
{
	delete m_impl;
}

template <concepts::connection Connection>
std::string_view basic_response<Connection>::version() const noexcept
{
	return m_impl->m_generator.version();
}

template <concepts::connection Connection>
basic_response<Connection> &basic_response<Connection>::set_status(status_enum status)
{
	return *this;
}

template <concepts::connection Connection>
basic_response<Connection> &basic_response<Connection>::auto_set(request_t &request)
{
	if( version() >= http_nt::version::v11 )
	{
		auto value = request.header(http_nt::header::transfer_encoding);
		if( value and strtls::to_lower(**value) == "chunked" )
			this->set_header(http_nt::header::transfer_encoding, "chunked");
	}
	return *this;
}

template <concepts::connection Connection>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Connection>::write(const const_buffer &body, Token &&token) noexcept
{

}

template <concepts::connection Connection>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Connection>::write(Token &&token) noexcept
{

}

template <concepts::connection Connection>
template <typename T, core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Connection>::send_file(T &&opt, Token &&token)
	requires file_opt_token<T>
{

}


} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H