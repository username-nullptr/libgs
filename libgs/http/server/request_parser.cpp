
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

#include "request_parser.h"
#include <libgs/core/algorithm/misc.h>
#include <libgs/core/string_vector.h>
#include <libgs/http/parser_base.h>
#include <ranges>

namespace libgs::http
{

class LIBGS_HTTP_TAPI request_parser::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(size_t init_buf_size) :
		m_parser(init_buf_size)
	{
		m_parser
		.on_parse_begin([this](std::string_view line_buf, error_code &error)
		{
			auto version = version::nan;
			auto request_line_parts = string_vector::from_string(line_buf, ' ');
			if( request_line_parts.size() != 3 or not strtls::to_upper(request_line_parts[2]).starts_with("HTTP/") )
			{
				error = parser_base::make_error_code(parse_errno::IRL);
				return version;
			}
			method_t method;
			try {
				method = from_method_string(request_line_parts[0]);
			}
			catch(const std::exception&)
			{
				error = parser_base::make_error_code(parse_errno::IHM);
				return version;
			}
			m_method = method;
			version = version_number(request_line_parts[2].substr(5,3));

			auto url_line = from_percent_encoding(request_line_parts[1]);
			auto pos = url_line.find('?');

			if( pos == std::string::npos )
				m_path = url_line;
			else
			{
				m_path = url_line.substr(0, pos);
				auto parameters_string = url_line.substr(pos + 1);

				for(auto &para_str : string_vector::from_string(parameters_string, '&'))
				{
					pos = para_str.find('=');
					if( pos == std::string::npos )
						m_parameters.emplace(para_str, para_str);
					else
						m_parameters.emplace(para_str.substr(0, pos), para_str.substr(pos+1));
				}
			}
			if( not m_path.starts_with("/") )
			{
				error = parser_base::make_error_code(parse_errno::IHP);
				return version;
			}
			auto n_it = std::unique(m_path.begin(), m_path.end(), [](char c0, char c1){
				return c0 == c1 and c0 == '/';
			});
			if( n_it != m_path.end() )
				m_path.erase(n_it, m_path.end());

			if( m_path.size() > 1 and m_path.ends_with("/") )
				m_path.pop_back();
			return version;
		})
		.on_parse_cookie([this](std::string_view line_buf, error_code &error)
		{
			auto list = string_vector::from_string(line_buf, ';');
			for(auto &statement : list)
			{
				statement = strtls::trimmed(statement);
				auto pos = statement.find('=');

				if( pos == std::string::npos )
				{
					error = parser_base::make_error_code(parse_errno::IHL);
					return ;
				}
				auto key = strtls::trimmed(statement.substr(0,pos));
				auto value = strtls::trimmed(statement.substr(pos+1));
				m_cookies[std::move(key)] = std::move(value);
			}
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
	parser_base m_parser;
	method_t m_method = method_t::get;

	std::string m_path {};
	http::parameters m_parameters {};
	path_args_t m_path_args {};
	cookie_values m_cookies {};

	bool m_keep_alive = true;
	bool m_support_gzip = false;
};

request_parser::request_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

request_parser::~request_parser()
{
	delete m_impl;
}

request_parser::request_parser(request_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

request_parser &request_parser::operator=(request_parser &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

bool request_parser::append(const const_buffer &buf, error_code &error)
{
	bool res = m_impl->m_parser.append(buf, error);
	if( not error )
		m_impl->set_attribute();
	return res;
}

bool request_parser::append(const const_buffer &buf)
{
	bool res = m_impl->m_parser.append(buf);
	m_impl->set_attribute();
	return res;
}

request_parser &request_parser::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

int32_t request_parser::path_match(std::string_view rule)
{
	auto rule_list = rule == "/" ?
		string_vector{{rule.data(), rule.size()}} :
		string_vector::from_string(rule, "/");

	auto path_list = string_vector::from_string(m_impl->m_path, "/");
	if( path_list.empty() )
		path_list.emplace_back("/");
	if( path_list.size() < rule_list.size() )
		return -1;

	path_args_t vector;
	size_t index = rule_list.size();

	for(auto &format : std::ranges::reverse_view(rule_list))
	{
		if( not format.starts_with('{') or not format.ends_with('}')  )
			break;
		else if( format.size() == 2 )
		{
			vector.emplace_back();
			--index;
			continue;
		}
		bool res = true;
		for(size_t i=1; i<format.size()-1; i++)
		{
			if( format[i] == '{' or format[i] == '}' )
			{
				res = false;
				break;
			}
		}
		if( res )
		{
			std::string key(format.c_str() + 1, format.size() - 2);
			vector.emplace_back(std::make_pair(std::move(key), value_t()));
			--index;
		}
	}
	std::reverse(vector.begin(), vector.end());

	auto rule_before = rule_list.join(0, index, "/");
	std::string path_before;

	index = path_list.size() - vector.size();
	if( vector.empty() )
		path_before = path_list.join("/");
	else
		path_before = path_list.join(0, index, "/");

	auto weight = wildcard_match(rule_before, path_before);
	if( weight < 0 )
		return -1;

	for(auto &[key,value] : vector)
	{
		ignore_unused(key);
		value = path_list[index++];
	}
	m_impl->m_path_args = std::move(vector);
	return weight;
}

method_t request_parser::method() const noexcept
{
	return m_impl->m_method;
}

std::string_view request_parser::path() const noexcept
{
	return m_impl->m_path;
}

version_t request_parser::version() const noexcept
{
	return m_impl->m_parser.version();
}

const parameters &request_parser::parameters() const noexcept
{
	return m_impl->m_parameters;
}

const request_parser::path_args_t &request_parser::path_args() const noexcept
{
	return m_impl->m_path_args;
}

const headers &request_parser::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

const cookie_values &request_parser::cookies() const noexcept
{
	return m_impl->m_cookies;
}

bool request_parser::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

bool request_parser::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

bool request_parser::can_read_from_device() const noexcept
{
	return m_impl->m_parser.can_read_from_device();
}

std::string request_parser::take_partial_body(size_t size)
{
	return m_impl->m_parser.take_partial_body(size);
}

std::string request_parser::take_body()
{
	return m_impl->m_parser.take_body();
}

bool request_parser::is_finished() const noexcept
{
	return m_impl->m_parser.is_finished();
}

bool request_parser::is_eof() const noexcept
{
	return m_impl->m_parser.is_eof();
}

request_parser &request_parser::reset()
{
	m_impl->m_parser.reset();
	m_impl->m_path.clear();
	m_impl->m_parameters.clear();
	m_impl->m_cookies.clear();
	return *this;
}

} //namespace libgs::http