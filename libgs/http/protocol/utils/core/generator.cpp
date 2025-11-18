
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

#include "generator.h"

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN generator<model::base>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;
	headers_t m_headers {{
		header::content_type,
		"text/plain; charset=utf-8"
	}};
	std::set<value_t> m_chunk_attributes {};
	size_t m_content_length = 0;
	state_t m_state {};
};

generator<model::base>::generator() :
	m_impl(new impl())
{

}

generator<model::base>::~generator()
{
	delete m_impl;
}

const headers &generator<model::base>::headers() const noexcept
{
	return m_impl->m_headers;
}

headers &generator<model::base>::headers() noexcept
{
	return m_impl->m_headers;
}

generator<model::base>& generator<model::base>::set_chunk_attribute(value_t attr) noexcept
{
	if( auto [it, inserted] = m_impl->m_chunk_attributes.emplace(std::move(attr)); not inserted )
	{
		m_impl->m_chunk_attributes.erase(it);
		m_impl->m_chunk_attributes.emplace(std::move(attr));
	}
	return *this;
}

generator<model::base>& generator<model::base>::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_chunk_attributes.erase(attr);
	return *this;
}

const std::set<value> &generator<model::base>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

generator<model::base> &generator<model::base>::reset()
{
	headers().clear();
	chunk_attributes().clear();
	m_impl->m_state = state_t::header;
	return *this;
}

std::string generator<model::base>::header_data(size_t body_size)
{
	if( state() != state_t::header )
		return {};

	auto &headers = this->headers();
	auto it = headers.find(header::content_length);

	if( it == headers.end() )
	{
		m_impl->m_content_length = body_size;
		m_impl->m_state = state_t::content_length;
		m_impl->m_headers[header::content_length] = m_impl->m_content_length;
	}
	else
	{
		m_impl->m_content_length = *it->second.get<size_t>().or_else();
		m_impl->m_state = state_t::content_length;
	}
	std::string buf;
	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf;
}

std::string generator<model::base>::body_data(const const_buffer &buffer)
{
	if( m_impl->m_state == state_t::header or m_impl->m_state == state_t::finish )
		return {};

	else if( m_impl->m_state == state_t::content_length )
	{
		size_t size = 0;
		if( m_impl->m_content_length > buffer.size() )
		{
			size = buffer.size();
			m_impl->m_content_length -= size;
		}
		else
		{
			size = m_impl->m_content_length;
			m_impl->m_content_length = 0;
			m_impl->m_state = state_t::finish;
		}
		return {static_cast<const char*>(buffer.data()), size};
	}
	std::string sum;
	if( m_impl->m_chunk_attributes.empty() )
		sum += std::format("{:X}\r\n", buffer.size());
	else
	{
		std::string attributes;
		for(auto &attr : m_impl->m_chunk_attributes)
			attributes += *attr + ";";

		m_impl->m_chunk_attributes.clear();
		attributes.pop_back();
		sum += std::format("{:X}; {}\r\n", buffer.size(), attributes);
	}
	return sum + std::string(static_cast<const char*>(buffer.data()), buffer.size()) + "\r\n";
}

std::string generator<model::base>::chunk_end_data(const headers_t &headers)
{
	if( m_impl->m_state != state_t::chunk )
		return {};

	m_impl->m_state = state_t::finish;
	std::string buf = "0\r\n";

	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf + "\r\n";
}

std::string generator<model::base>::header_data()
{
	return header_data(0);
}

std::string generator<model::base>::chunk_end_data()
{
	return chunk_end_data({});
}

std::set<value> &generator<model::base>::chunk_attributes() noexcept
{
	return m_impl->m_chunk_attributes;
}

generator<model::base>::state_t  generator<model::base>::state() const noexcept
{
	return m_impl->m_state;
}

version_enum generator_v10<model::base>::version() const noexcept
{
	return version_enum::v10;
}

std::string generator_v11<model::base>::header_data(size_t body_size)
{
	if( state() != state_t::header )
		return {};

	auto &headers = this->headers();
	if( auto it = headers.find(header::content_length); it == headers.end() )
	{
		it = headers.find(header::transfer_encoding);
		if( it != headers.end() and strtls::to_lower(*it->second) == "chunked" )
			m_impl->m_state = state_t::chunk;
		else
		{
			m_impl->m_content_length = body_size;
			m_impl->m_state = state_t::content_length;
			m_impl->m_headers[header::content_length] = m_impl->m_content_length;
		}
	}
	else
	{
		m_impl->m_content_length = *it->second.get<size_t>().or_else();
		m_impl->m_state = state_t::content_length;
	}
	std::string buf;
	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf;
}

version_enum generator_v11<model::base>::version() const noexcept
{
	return version_enum::v11;
}

} //namespace libgs::http::protocol