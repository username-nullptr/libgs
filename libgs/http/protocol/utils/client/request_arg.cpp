
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#include "request_arg.h"

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN request_arg::impl
{
public:
	impl() = default;
	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	headers_t m_headers {{
		header_t::content_type,
		"text/plain; charset=utf-8"
	}};
	cookies_t m_cookies {};
	std::set<value_t> m_chunk_attributes {};
};

request_arg::request_arg() :
	m_impl(new impl())
{

}

request_arg::~request_arg()
{
	delete m_impl;
}

request_arg::request_arg(const request_arg &other) noexcept :
	m_impl(new impl(*other.m_impl))
{

}

request_arg &request_arg::operator=(const request_arg &other) noexcept
{
	*m_impl = *other.m_impl;
	return *this;
}

request_arg::request_arg(request_arg &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

request_arg &request_arg::operator=(request_arg &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl({});
	return *this;
}

const headers &request_arg::headers() const noexcept
{
	return m_impl->m_headers;
}

headers &request_arg::headers() noexcept
{
	return m_impl->m_headers;
}

const cookie_values &request_arg::cookies() const noexcept
{
	return m_impl->m_cookies;
}

cookie_values &request_arg::cookies() noexcept
{
	return m_impl->m_cookies;
}

request_arg &request_arg::set_chunk_attribute(value_t attr) noexcept
{
	if( auto [it, inserted] = m_impl->m_chunk_attributes.emplace(std::move(attr)); not inserted )
	{
		m_impl->m_chunk_attributes.erase(it);
		m_impl->m_chunk_attributes.emplace(std::move(attr));
	}
	return *this;
}

request_arg &request_arg::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_chunk_attributes.erase(attr);
	return *this;
}

const std::set<value> &request_arg::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

std::set<value> &request_arg::chunk_attributes() noexcept
{
	return m_impl->m_chunk_attributes;
}

} //namespace libgs::http::protocol