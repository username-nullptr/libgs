
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

#include "parser.h"
#include <libgs/core/string_vector.h>

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN parser<model::client>::impl
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

			if( request_line_parts.size() < 2 or not strtls::to_upper(request_line_parts[0]).starts_with("HTTP/") )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IRPYL)
				);
			}
			result = version::from_string(request_line_parts[0].substr(5,3));
			m_status = *strtls::to_arith<status_enum>(request_line_parts[1]).or_else();

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
				pos = statement.find('=');

				if( pos == std::string::npos )
					return base_parser::make_error_code(parse_errno::ICL);

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
		if( it == headers.end() )
			m_keep_alive = m_parser.version() != version::v10;
		else
			m_keep_alive = strtls::to_lower(it->second.to_string()) != "close";

		it = headers.find(header::accept_encoding);
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
	status_enum m_status = status::ok;

	std::string m_description = status::description<status::ok>();
	cookies_t m_cookies {};

	bool m_keep_alive = false;
	bool m_support_gzip = false;
};

parser<model::client>::parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

parser<model::client>::~parser()
{
	delete m_impl;
}

parser<model::client>::parser(parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

parser<model::client> &parser<model::client>::operator=(parser &&other) noexcept
{
	if( this == &other )
        return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

sys_expected<bool> parser<model::client>::append(const const_buffer &buf)
{
	return m_impl->m_parser.append(buf);
}

parser<model::client> &parser<model::client>::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

version_enum parser<model::client>::version() const noexcept
{
	return m_impl->m_parser.version();
}

status_enum parser<model::client>::status() const noexcept
{
	return m_impl->m_status;
}

const headers &parser<model::client>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

const cookies &parser<model::client>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

const std::vector<value> &parser<model::client>::chunk_attributes() const noexcept
{
	// TODO ... ...
	static std::vector<value> tmp;
	return tmp;
}

bool parser<model::client>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

bool parser<model::client>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

std::string parser<model::client>::take_partial_body(size_t size)
{
	return m_impl->m_parser.take_partial_body(size);
}

std::string parser<model::client>::take_body()
{
	return m_impl->m_parser.take_body();
}

parser<model::client>::stage_t parser<model::client>::stage() const noexcept
{
	return m_impl->m_parser.stage();
}

parser<model::client> &parser<model::client>::reset()
{
	m_impl->m_parser.reset();
	m_impl->m_status = status::ok;
	m_impl->m_description = status::description<status::ok>();
	m_impl->m_cookies.clear();
	m_impl->m_keep_alive = false;
	m_impl->m_support_gzip = false;
	return *this;
}

} //namespace libgs::http::protocol