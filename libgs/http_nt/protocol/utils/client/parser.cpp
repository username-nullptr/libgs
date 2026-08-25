
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

#include "parser.h"
#include <libgs/http_nt/protocol/utils/core/parser.h>
#include <libgs/core/string_vector.h>

namespace libgs::http_nt
{

class LIBGS_DECL_HIDDEN parser<protocol_model::client>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(size_t init_buf_size) :
		m_parser(init_buf_size)
	{
		m_parser
		.on_parse_begin([this](std::string_view line_buf)
		{
			sys_expected<version_enum> result = static_cast<version_enum>(0);
			auto request_line_parts = string_vector::from_string(line_buf, ' ');

			if( request_line_parts.size() < 2 or
				not strtls::to_upper(request_line_parts[0]).starts_with("HTTP/") )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IRPYL)
				);
			}
			try {
				result = version::from_string(request_line_parts[0].substr(5,3));
			}
			catch(const std::exception&)
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IRPYL)
				);
			}
			auto status_value = strtls::to_arith<status_enum>(request_line_parts[1]);
			if( not status_value )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IHSC)
				);
			}
			m_status = *status_value;

			if( m_status == static_cast<status_enum>(0) )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IHSC)
				);
			}
			if( request_line_parts.size() > 2 )
				m_description = request_line_parts.join(2, ' ');
			else
				m_description = status::description(m_status);
			return result;
		})
		.on_parse_cookie([this](std::string_view line_buf)
		{
			auto vector = string_vector::from_string(line_buf, ';');
			if( vector.empty() )
				return base_parser::make_error_code(parse_errno::ICL);

			vector[0] = strtls::trimmed(vector[0]);
			auto pos = vector[0].find('=');

			if( pos == std::string::npos )
				return base_parser::make_error_code(parse_errno::ICL);

			auto key = strtls::trimmed(vector[0].substr(0, pos));
			auto value = strtls::trimmed(vector[0].substr(pos + 1));
			auto &cookie = m_cookies[std::move(key)] = std::move(value);

			for(size_t i=1; i<vector.size(); i++)
			{
				auto &statement = vector[i];
				statement = strtls::trimmed(statement);

				if( statement.empty() )
					continue;

				pos = statement.find('=');
				if( pos == std::string::npos )
				{
					cookie.set_attribute(std::move(statement), true);
					continue;
				}
				key = strtls::trimmed(statement.substr(0,pos));
				value = strtls::trimmed(statement.substr(pos+1));
				cookie.set_attribute(std::move(key), std::move(value));
			}
			return error_code();
		});
	}

public:
	void set_attribute()
	{
		auto headers = m_parser.headers();
		auto it = headers.find(header::connection);
		m_keep_alive = m_parser.version() != version::v10;
		if( it != headers.end() )
		{
			for(auto &str : string_vector::from_string(it->second.to_string(), ','))
			{
				auto value = strtls::to_lower(strtls::trimmed(str));
				if( value == "close" )
					m_keep_alive = false;
				else if( value == "keep-alive" )
					m_keep_alive = true;
			}
		}

		it = headers.find(header::content_encoding);
		if( it == headers.end() )
		{
			m_support_gzip = false;
			return ;
		}
		for(auto &str : string_vector::from_string(it->second.to_string(), ","))
		{
			if( strtls::to_lower(strtls::trimmed(str)) == "gzip" )
			{
				m_support_gzip = true;
				break;
			}
		}
	}

public:
	base_parser m_parser;
	status_enum m_status = status::none;

	std::string m_description = status::description<status::none>();
	cookies_t m_cookies {};

	bool m_keep_alive = false;
	bool m_support_gzip = false;
};

parser<protocol_model::client>::parser(size_t init_buf_size) :
	const_headers(nullptr),
	const_cookies(nullptr),
	const_chunk_attributes(nullptr),
	m_impl(new impl(init_buf_size))
{
	m_headers = &m_impl->m_parser.headers();
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_parser.chunk_attributes();
}

bool parser<protocol_model::client>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

bool parser<protocol_model::client>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

std::string parser<protocol_model::client>::take_partial_body(size_t size)
{
	return m_impl->m_parser.take_partial_body(size);
}

std::string parser<protocol_model::client>::take_body()
{
	return m_impl->m_parser.take_body();
}

parser<protocol_model::client>::~parser()
{
	delete m_impl;
}

parser<protocol_model::client>::parser(parser &&other) noexcept :
	const_headers(nullptr),
	const_cookies(nullptr),
	const_chunk_attributes(nullptr),
	m_impl(other.m_impl)
{
	m_headers = &m_impl->m_parser.headers();
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_parser.chunk_attributes();

	other.m_impl = new impl(0xFFFF);
	other.m_headers = &other.m_impl->m_parser.headers();
	other.m_cookies = &other.m_impl->m_cookies;
	other.m_chunk_attributes = &other.m_impl->m_parser.chunk_attributes();
}

parser<protocol_model::client> &parser<protocol_model::client>::operator=(parser &&other) noexcept
{
	if( this == &other )
		return *this;

	delete m_impl;
	m_impl = other.m_impl;
	m_headers = &m_impl->m_parser.headers();
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_parser.chunk_attributes();

	other.m_impl = new impl(0xFFFF);
	other.m_headers = &other.m_impl->m_parser.headers();
	other.m_cookies = &other.m_impl->m_cookies;
	other.m_chunk_attributes = &other.m_impl->m_parser.chunk_attributes();
	return *this;
}

sys_expected<bool> parser<protocol_model::client>::append(const const_buffer &buf)
{
	auto expected = m_impl->m_parser.append(buf);
	if( expected and m_impl->m_parser.stage() != stage::header )
		m_impl->set_attribute();
	return expected;
}

parser<protocol_model::client> &parser<protocol_model::client>::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

version_enum parser<protocol_model::client>::version() const noexcept
{
	return m_impl->m_parser.version();
}

status_enum parser<protocol_model::client>::status() const noexcept
{
	return m_impl->m_status;
}

parser<protocol_model::client>::stage_t parser<protocol_model::client>::stage() const noexcept
{
	return m_impl->m_parser.stage();
}

parser<protocol_model::client> &parser<protocol_model::client>::reset()
{
	m_impl->m_parser.reset();
	m_impl->m_status = status::none;
	m_impl->m_description = status::description<status::none>();
	m_impl->m_cookies.clear();
	m_impl->m_keep_alive = false;
	m_impl->m_support_gzip = false;
	return *this;
}

} //namespace libgs::http_nt
