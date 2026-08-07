
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

namespace libgs::http_nt
{

class LIBGS_DECL_HIDDEN generator<protocol_model::base>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;

	// TODO: It will be used in the parser ... ...
	// [[nodiscard]] body_norms_t do_correct_body_norms() const noexcept
	// {
	// 	if( m_state == state_t::header or m_state == state_t::finish )
	// 		return basic_body_norms();
	//
	// 	auto it = m_headers.find(header_t::content_type);
	// 	if( it == m_headers.end() or not contains_header(header_t::accept_ranges, "bytes") )
	// 		return basic_body_norms();
	//
	// 	constexpr std::string_view prefix =
	// 		"multipart/byteranges; boundary=";
	//
	// 	if( it->second->starts_with(prefix) )
	// 	{
	// 		return multipart_body_norms {
	// 			.boundary = it->second->substr(prefix.size())
	// 		};
	// 	}
	// 	it = m_headers.find(header_t::content_range);
	// 	if( it == m_headers.end() )
	// 		return basic_body_norms();
	//
	// 	auto &value = it->second;
	// 	if( value->size() < 5 )
	// 		return basic_body_norms();
	//
	// 	auto dash_pos = value->find('-');
	// 	if( dash_pos == std::string::npos )
	// 		return basic_body_norms();
	//
	// 	auto begin = strtls::to_arith<size_t>(
	// 		value->substr(0, dash_pos)
	// 	);
	// 	if( not begin )
	// 		return basic_body_norms();
	//
	// 	auto slash_pos = value->find('/', dash_pos + 1);
	// 	if( slash_pos == std::string::npos )
	// 		return basic_body_norms();
	//
	// 	auto end = strtls::to_arith<size_t>(
	// 		value->substr(dash_pos + 1, slash_pos - dash_pos - 1)
	// 	);
	// 	if( not end )
	// 		return basic_body_norms();
	//
	// 	auto total = strtls::to_arith<size_t>(
	// 		value->substr(slash_pos + 1)
	// 	);
	// 	if( not total )
	// 		return basic_body_norms();
	//
	// 	return range_body_norms {
	// 		*begin, *total
	// 	};
	// }

	headers_t m_headers {{
		header::content_type,
		"text/plain; charset=utf-8"
	}};

	values_t m_chunk_attributes {};
	size_t m_content_length = 0;

	state_t m_state {};
	body_norms_t m_body_norms {};
};

generator<protocol_model::base>::generator() :
	mutable_headers(nullptr),
	mutable_chunk_attributes(nullptr),
	m_impl(new impl())
{
	m_headers = &m_impl->m_headers;
	m_chunk_attributes = &m_impl->m_chunk_attributes;
}

generator<protocol_model::base>::~generator()
{
	delete m_impl;
}

generator<protocol_model::base> &generator<protocol_model::base>::reset()
{
	headers().clear();
	chunk_attributes().clear();
	m_impl->m_state = state_t::header;
	return *this;
}

std::string generator<protocol_model::base>::header_data(size_t body_size) noexcept
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

std::string generator<protocol_model::base>::body_data(const const_buffer &buffer) noexcept
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

std::string generator<protocol_model::base>::chunk_end_data(const headers_t &headers) noexcept
{
	if( m_impl->m_state != state_t::chunk )
		return {};

	m_impl->m_state = state_t::finish;
	std::string buf = "0\r\n";

	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf + "\r\n";
}

std::string generator<protocol_model::base>::header_data() noexcept
{
	return header_data(0);
}

std::string generator<protocol_model::base>::chunk_end_data() noexcept
{
	return chunk_end_data({});
}

generator<protocol_model::base>::state_t generator<protocol_model::base>::state() const noexcept
{
	return m_impl->m_state;
}

version_enum generator_v10<protocol_model::base>::version() const noexcept
{
	return version_enum::v10;
}

std::string generator_v11<protocol_model::base>::header_data(size_t body_size) noexcept
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

version_enum generator_v11<protocol_model::base>::version() const noexcept
{
	return version_enum::v11;
}

} //namespace libgs::http_nt
