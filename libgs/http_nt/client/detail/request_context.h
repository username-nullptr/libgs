
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_REQUEST_CONTEXT_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_REQUEST_CONTEXT_H

namespace libgs::http_nt
{

template <method_enum Method, concepts::connection Connection, version_enum Version>
class LIBGS_HTTP_NT_TAPI basic_request_context<Method,Connection,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl(connection_t &&connection, url_t url, request_arg_t arg) :
		m_connection(connection), m_generator(std::move(url), std::move(arg)) {}

	impl(impl &&other) = default;
	impl &operator=(impl &&other) = default;

public:

public:
	connection_t m_connection;
	generator_t m_generator;
};

template <method_enum Method, concepts::connection Connection, version_enum Version>
basic_request_context<Method,Connection,Version>::basic_request_context
(connection_t &&connection, url_t url, request_arg_t arg) :
	mutable_headers<basic_request_context>(nullptr),
	mutable_cookies<value,basic_request_context>(nullptr),
	mutable_chunk_attributes<basic_request_context>(nullptr),
	m_impl(new impl(std::move(connection), std::move(url), std::move(arg)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <method_enum Method, concepts::connection Connection, version_enum Version>
basic_request_context<Method,Connection,Version>::~basic_request_context() = default;

template <method_enum Method, concepts::connection Connection, version_enum Version>
basic_request_context<Method,Connection,Version>::basic_request_context
(basic_request_context &&other) noexcept :
	mutable_headers<basic_request_context>(nullptr),
	mutable_cookies<value,basic_request_context>(nullptr),
	mutable_chunk_attributes<basic_request_context>(nullptr),
	m_impl(new impl(std::move(*other.m_impl)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <method_enum Method, concepts::connection Connection, version_enum Version>
basic_request_context<Method,Connection,Version>&
basic_request_context<Method,Connection,Version>::operator=(basic_request_context &&other) noexcept
{
	if( &other != this )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_REQUEST_CONTEXT_H