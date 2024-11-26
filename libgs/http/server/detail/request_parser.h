
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H
#define LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H

#include <libgs/http/parser_base.h>
#include <libgs/core/string_list.h>
#include <ranges>

namespace libgs::http { namespace detail
{

template <core_concepts::char_type T>
struct _request_parser_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *comma = __VA_ARGS__##",";

template <>
struct _request_parser_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _request_parser_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	struct string_pool : detail::string_pool<CharT>, detail::_request_parser_static_string<CharT> {};
	using string_list_t = basic_string_list<CharT>;
	using parser_t = basic_parser_base<CharT>;

public:
	explicit impl(size_t init_buf_size) :
		m_parser(init_buf_size)
	{
		m_parser
		.on_parse_begin([this](std::string_view line_buf, error_code &error)
		{
			std::string version;
			auto request_line_parts = string_list::from_string(line_buf, ' ');
			if( request_line_parts.size() != 3 or not str_to_upper(request_line_parts[2]).starts_with("HTTP/") )
			{
				error = parser_t::make_error_code(parse_errno::IRL);
				return version;
			}
			http::method method;
			try {
				method = from_method_string(request_line_parts[0]);
			}
			catch(std::exception&)
			{
				error = parser_t::make_error_code(parse_errno::IHM);
				return version;
			}
			m_method  = method;
			version = mbstoxx<CharT>(request_line_parts[2].substr(5,3));

			auto url_line = from_percent_encoding(request_line_parts[1]);
			auto pos = url_line.find('?');

			if( pos == std::string::npos )
				m_path = mbstoxx<CharT>(url_line);
			else
			{
				m_path = mbstoxx<CharT>(url_line.substr(0, pos));
				auto parameters_string = url_line.substr(pos + 1);

				for(auto &para_str : string_list::from_string(parameters_string, '&'))
				{
					pos = para_str.find('=');
					if( pos == std::string::npos )
						m_parameters.emplace(mbstoxx<CharT>(para_str), mbstoxx<CharT>(para_str));
					else
						m_parameters.emplace(mbstoxx<CharT>(para_str.substr(0, pos)), mbstoxx<CharT>(para_str.substr(pos+1)));
				}
			}
			if( not m_path.starts_with(string_pool::root) )
			{
				error = parser_t::make_error_code(parse_errno::IHP);
				return version;
			}
			auto n_it = std::unique(m_path.begin(), m_path.end(), [](CharT c0, CharT c1){
				return c0 == c1 and c0 == 0x2F/*/*/;
			});
			if( n_it != m_path.end() )
				m_path.erase(n_it, m_path.end());

			if( m_path.size() > 1 and m_path.ends_with(string_pool::root) )
				m_path.pop_back();
			return version;
		})
		.on_parse_cookie([this](std::string_view line_buf, error_code &error)
		{
			auto list = string_list::from_string(line_buf, ';');
			for(auto &statement : list)
			{
				statement = str_trimmed(statement);
				auto pos = statement.find('=');

				if( pos == std::string::npos )
				{
					error = parser_t::make_error_code(parse_errno::IHL);
					return ;
				}
				auto key = str_trimmed(statement.substr(0,pos));
				auto value = str_trimmed(statement.substr(pos+1));
				m_cookies[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
			}
		});
	}

public:
	void set_attribute()
	{
		auto headers = m_parser.headers();
		auto it = headers.find(basic_header<CharT>::connection);
		if( it == headers.end() )
			m_keep_alive = m_parser.version() != string_pool::v_1_0;
		else
			m_keep_alive = str_to_lower(it->second.to_string()) != string_pool::close;

		it = headers.find(basic_header<CharT>::accept_encoding);
		if( it == headers.end() )
		{
			m_support_gzip = false;
			return ;
		}
		for(auto &str : string_list_t::from_string(it->second.to_string(), string_pool::comma))
		{
			if( str_to_lower(str_trimmed(str)) == string_pool::gzip )
			{
				m_support_gzip = true;
				break;
			}
		}
	}

public:
	parser_t m_parser;
	http::method m_method = http::method::GET;

	string_t m_path {};
	parameters_t m_parameters {};
	path_args_t m_path_args {};
	cookies_t m_cookies {};

	bool m_keep_alive = true;
	bool m_support_gzip = false;
};

template <core_concepts::char_type CharT>
basic_request_parser<CharT>::basic_request_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <core_concepts::char_type CharT>
basic_request_parser<CharT>::~basic_request_parser()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_request_parser<CharT>::basic_request_parser(basic_request_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <core_concepts::char_type CharT>
basic_request_parser<CharT> &basic_request_parser<CharT>::operator=(basic_request_parser &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf, error_code &error)
{
	bool res = m_impl->m_parser.append(buf, error);
	if( not error )
		m_impl->set_attribute();
	return res;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf)
{
	bool res = m_impl->m_parser.append(buf);
	m_impl->set_attribute();
	return res;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::operator<<(std::string_view buf)
{
	return append(buf);
}

template <core_concepts::char_type CharT>
int32_t basic_request_parser<CharT>::path_match(string_view_t rule)
{
	constexpr const CharT *root = detail::string_pool<CharT>::root;
	using string_list_t = basic_string_list<CharT>;

	auto rule_list = string_list_t::from_string(rule, root/*/*/);
	auto path_list = string_list_t::from_string(m_impl->m_path, root/*/*/);

	if( path_list.empty() )
		path_list.emplace_back(root);
	if( path_list.size() < rule_list.size() )
		return -1;

	path_args_t vector;
	size_t index = rule_list.size();

	for(auto &format : std::ranges::reverse_view(rule_list))
	{
		if( not format.starts_with(0x7B/*{*/) or not format.ends_with(0x7D/*}*/)  )
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
			if( format[i] == 0x7B/*{*/ or format[i] == 0x7D/*}*/ )
			{
				res = false;
				break;
			}
		}
		if( res )
		{
			string_t key(format.c_str() + 1, format.size() - 2);
			vector.emplace_back(std::make_pair(std::move(key), value_t()));
			--index;
		}
	}
	std::reverse(vector.begin(), vector.end());

	auto rule_before = rule_list.join(0, index, root/*/*/);
	string_t path_before;

	index = path_list.size() - vector.size();
	if( vector.empty() )
		path_before = path_list.join(root/*/*/);
	else
		path_before = path_list.join(0, index, root/*/*/);

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

template <core_concepts::char_type CharT>
http::method basic_request_parser<CharT>::method() const noexcept
{
	return m_impl->m_method;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_request_parser<CharT>::path() const noexcept
{
	return m_impl->m_path;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_request_parser<CharT>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <core_concepts::char_type CharT>
const typename basic_request_parser<CharT>::parameters_t &basic_request_parser<CharT>::parameters() const noexcept
{
	return m_impl->m_parameters;
}

template <core_concepts::char_type CharT>
const typename basic_request_parser<CharT>::path_args_t &basic_request_parser<CharT>::path_args() const noexcept
{
	return m_impl->m_path_args;
}

template <core_concepts::char_type CharT>
const typename basic_request_parser<CharT>::headers_t &basic_request_parser<CharT>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

template <core_concepts::char_type CharT>
const typename basic_request_parser<CharT>::cookies_t &basic_request_parser<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::can_read_from_device() const noexcept
{
	return m_impl->m_parser.can_read_from_device();
}

template <core_concepts::char_type CharT>
std::string basic_request_parser<CharT>::take_partial_body(size_t size)
{
	return m_impl->m_parser.take_partial_body(size);
}

template <core_concepts::char_type CharT>
std::string basic_request_parser<CharT>::take_body()
{
	return m_impl->m_parser.take_body();
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::is_finished() const noexcept
{
	return m_impl->m_parser.is_finished();
}

template <core_concepts::char_type CharT>
bool basic_request_parser<CharT>::is_eof() const noexcept
{
	return m_impl->m_parser.is_eof();
}

template <core_concepts::char_type CharT>
basic_request_parser<CharT> &basic_request_parser<CharT>::reset()
{
	m_impl->m_parser.reset();
	m_impl->m_path.clear();
	m_impl->m_parameters.clear();
	m_impl->m_cookies.clear();
	return *this;
}

} //namespace libgs::http

#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H
