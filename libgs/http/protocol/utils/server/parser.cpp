
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

#include "parser.h"
#include <libgs/core/algorithm/misc.h>
#include <libgs/core/string_vector.h>
#include <ranges>

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN parser<model::server>::impl
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

			if( request_line_parts.size() != 3 or not strtls::to_upper(request_line_parts[2]).starts_with("HTTP/") )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IREQL)
				);
			}
			method_enum method;
			try {
				method = method::from_string(request_line_parts[0]);
			}
			catch(const std::exception&)
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IHM)
				);
			}
			m_method = method;
			result = version::from_string(request_line_parts[2].substr(5,3));

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
						m_parameters.emplace_back(para_str, para_str);
					else
						m_parameters.emplace_back(para_str.substr(0, pos), para_str.substr(pos+1));
				}
			}
			if( not m_path.starts_with("/") )
			{
				return result.despair (
					base_parser::make_error_code(parse_errno::IHP)
				);
			}
			auto n_it = std::unique(m_path.begin(), m_path.end(), [](char c0, char c1){
				return c0 == c1 and c0 == '/';
			});
			if( n_it != m_path.end() )
				m_path.erase(n_it, m_path.end());

			if( m_path.size() > 1 and m_path.ends_with("/") )
				m_path.pop_back();
			return result;
		})
		.on_parse_cookie([this](std::string_view line_buf)
		{
			auto vector = string_vector::from_string(line_buf, ';');
			if( vector.empty() )
				return base_parser::make_error_code(parse_errno::ICL);

			for(auto &statement : vector)
			{
				statement = strtls::trimmed(statement);
				auto pos = statement.find('=');

				if( pos == std::string::npos )
					return base_parser::make_error_code(parse_errno::ICL);

				auto key = strtls::trimmed(statement.substr(0,pos));
				auto value = strtls::trimmed(statement.substr(pos+1));
				m_cookies[std::move(key)] = std::move(value);
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
	method_enum m_method = method_enum::get;

	std::string m_path {};
	parameters_t m_parameters {};
	path_args_t m_path_args {};
	cookie_values m_cookies {};

	bool m_keep_alive = false;
	bool m_support_gzip = false;
};

parser<model::server>::parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

parser<model::server>::~parser()
{
	delete m_impl;
}

parser<model::server>::parser(parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

parser<model::server> &parser<model::server>::operator=(parser &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

sys_expected<bool> parser<model::server>::append(const const_buffer &buf)
{
	return m_impl->m_parser.append(buf).transform([this](bool finished)
	{
		m_impl->set_attribute();
		return finished;
	});
}

parser<model::server> &parser<model::server>::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

int32_t parser<model::server>::path_match(std::string_view rule)
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

method_enum parser<model::server>::method() const noexcept
{
	return m_impl->m_method;
}

std::string_view parser<model::server>::path() const noexcept
{
	return m_impl->m_path;
}

version_enum parser<model::server>::version() const noexcept
{
	return m_impl->m_parser.version();
}

optional<value> parser<model::server>::parameter(size_t index) const
{
	if( index >= parameters().size() )
	{
		throw runtime_error (
			"libgs::http::parser<model::server>::parameter: index out of range."
		);
	}
	return parameters()[index].second;
}

const parameters &parser<model::server>::parameters() const noexcept
{
	return m_impl->m_parameters;
}

const parser<model::server>::headers_t &parser<model::server>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

const cookie_values &parser<model::server>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

optional<value> parser<model::server>::path_arg(size_t index) const
{
	if( index >= path_args().size() )
	{
		throw runtime_error (
			"libgs::http::parser<model::server>::path_arg: index out of range."
		);
	}
	return path_args()[index].second;
}

const parser<model::server>::path_args_t &parser<model::server>::path_args() const noexcept
{
	return m_impl->m_path_args;
}

bool parser<model::server>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

bool parser<model::server>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

bool parser<model::server>::can_read_from_device() const noexcept
{
	return m_impl->m_parser.can_read_from_device();
}

std::string parser<model::server>::take_partial_body(size_t size)
{
	return m_impl->m_parser.take_partial_body(size);
}

std::string parser<model::server>::take_body()
{
	return m_impl->m_parser.take_body();
}

bool parser<model::server>::is_finished() const noexcept
{
	return m_impl->m_parser.is_finished();
}

bool parser<model::server>::is_eof() const noexcept
{
	return m_impl->m_parser.is_eof();
}

parser<model::server> &parser<model::server>::reset()
{
	m_impl->m_parser.reset();
	m_impl->m_path.clear();
	m_impl->m_parameters.clear();
	m_impl->m_cookies.clear();
	m_impl->m_keep_alive = false;
	m_impl->m_support_gzip = false;
	return *this;
}

} //namespace libgs::http::protocol