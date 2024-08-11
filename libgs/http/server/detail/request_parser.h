
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

#include <libgs/http/basic/parser.h>
#include <ranges>

namespace libgs::http
{

namespace detail
{

template <concept_char_type T>
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

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	struct string_pool : detail::string_pool<CharT>, detail::_request_parser_static_string<CharT> {};

public:
	using string_t = std::basic_string<CharT>;
	using headers_t = basic_headers<CharT>;

	using cookies_t = std::map<std::basic_string<CharT>, basic_value<CharT>, basic_less_case_insensitive<CharT>>;
	using parameters_t = basic_parameters<CharT>;

public:
	explicit impl(size_t init_buf_size) {
		m_src_buf.reserve(init_buf_size);
	}

public:
	[[nodiscard]] bool parse_header(error_code &error)
	{
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() < 1024 )
					break;
				else if( m_state == state::waiting_request )
					error = error_code(PE_RLTL, s_error_category);
				else if( m_state == state::reading_headers )
					error = error_code(PE_HLTL, s_error_category);
				break;
			}
			auto line_buf = m_src_buf.substr(0, pos);
			m_src_buf.erase(0, pos + 2);

			if( m_state == state::waiting_request )
			{
				state_handler_waiting_request(line_buf, error);
				if( error )
					break;
			}
			else if( m_state == state::reading_headers )
			{
				if( state_handler_reading_headers(line_buf, error) )
					return true;
				else if( error )
					break;
			}
		}
		while( not m_src_buf.empty() );
		return false;
	}

	void parse_length() noexcept
	{
		auto tsize = m_state_context;
		auto rsize = m_partial_body.size() + m_src_buf.size();
		rsize = rsize > tsize ? tsize - m_partial_body.size() : m_src_buf.size();

		m_partial_body += std::string(m_src_buf.c_str(), rsize);
		m_src_buf.clear();

		m_state = tsize > m_partial_body.size() ? state::reading_length : state::finished;
	}

	bool parse_chunked(error_code &error)
	{
		std::size_t _size = 0;
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() >= 1024 )
					error = error_code(PE_HLTL, s_error_category);
				break;
			}
			auto line_buf = m_src_buf.substr(0, pos + 2);
			m_src_buf.erase(0, pos + 2);

			if( m_state == state::chunked_wait_size )
			{
				line_buf.erase(pos);
				pos = line_buf.find(';');

				if( pos != std::string::npos )
					line_buf.erase(pos);

				if( line_buf.size() > 16 )
				{
					error = error_code(PE_SFE, s_error_category);
					break;
				}
				try {
					_size = ston<size_t>(line_buf, 16);
				}
				catch(...) {
					error = error_code(PE_SFE, s_error_category);
					break;
				}
				m_state = _size == 0 ? state::chunked_wait_headers : state::chunked_wait_content;
			}
			else if( m_state == state::chunked_wait_content )
			{
				line_buf.erase(pos);
				if( _size < line_buf.size() )
					_size = line_buf.size();
				else
					m_state = state::chunked_wait_size;
				m_partial_body += line_buf;
			}
			else if( m_state == state::chunked_wait_headers )
			{
				if( line_buf == "\r\n" )
				{
					m_state = state::finished;
					m_src_buf.clear();
					return true;
				}
				auto colon_index = line_buf.find(':');
				if( colon_index == std::string::npos )
				{
					error = error_code(PE_SFE, s_error_category);
					break;
				}
				header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
							  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))), error);
			}
		}
		while( not m_src_buf.empty() );
		return false;
	}

public:
	void reset()
	{
		m_state = state::waiting_request;
		m_src_buf.clear();

		m_version.clear();
		m_path.clear();
		m_parameters.clear();

		m_headers.clear();
		m_cookies.clear();
		m_partial_body.clear();
	}

private:
	void state_handler_waiting_request(std::string_view line_buf, error_code &error)
	{
		auto request_line_parts = string_list::from_string(line_buf, ' ');
		if( request_line_parts.size() != 3 or not str_to_upper(request_line_parts[2]).starts_with("HTTP/") )
		{
			m_src_buf.clear();
			error = error_code(PE_IRL, s_error_category);
			return ;
		}
		http::method method;
		try {
			method = from_method_string(request_line_parts[0]);
		}
		catch(std::exception&)
		{
			m_src_buf.clear();
			error = error_code(PE_IHM, s_error_category);
			return ;
		}
		m_method  = method;
		m_version = mbstoxx<CharT>(request_line_parts[2].substr(5,3));

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
			m_src_buf.clear();
			error = error_code(PE_IHP, s_error_category);
			return ;
		}
		auto n_it = std::unique(m_path.begin(), m_path.end(), [](CharT c0, CharT c1){
			return c0 == c1 and c0 == 0x2F/*/*/;
		});
		if( n_it != m_path.end() )
			m_path.erase(n_it, m_path.end());

		if( m_path.size() > 1 and m_path.ends_with(string_pool::root) )
			m_path.pop_back();

		m_state = state::reading_headers;
	}

	[[nodiscard]] bool state_handler_reading_headers(std::string_view line_buf, error_code &error)
	{
		if( line_buf.empty() )
			return set_read_body_state(error);

		auto colon_index = line_buf.find(':');
		if( colon_index == std::string::npos )
		{
			reset();
			error = error_code(PE_IHL, s_error_category);
			return false;
		}
		header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
					  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))), error);
		return false;
	}

private:
	void header_insert(std::string key, std::string value, error_code &error)
	{
		if( key != "cookie" )
		{
			m_headers[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
			return ;
		}
		auto list = string_list::from_string(value, ';');
		for(auto &statement : list)
		{
			statement = str_trimmed(statement);
			auto pos = statement.find('=');

			if( pos == std::string::npos )
			{
				error = error_code(PE_IHL, s_error_category);
				return ;
			}
			key = str_trimmed(statement.substr(0,pos));
			value = str_trimmed(statement.substr(pos+1));
			m_cookies[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
		}
	}

	bool set_read_body_state(error_code &error)
	{
		auto it = m_headers.find(basic_header<CharT>::connection);
		if( it == m_headers.end() )
			m_keep_alive = m_version != string_pool::v_1_0;
		else
			m_keep_alive = str_to_lower(it->second.to_string()) != string_pool::close;

		it = m_headers.find(basic_header<CharT>::accept_encoding);
		if( it == m_headers.end() )
			m_support_gzip = false;
		else
		{
			for(auto &str : basic_string_list<CharT>::from_string(it->second.to_string(), string_pool::comma))
			{
				if( str_to_lower(str_trimmed(str)) == string_pool::gzip )
				{
					m_support_gzip = true;
					break;
				}
			}
		}
		it = m_headers.find(basic_header<CharT>::content_length);
		if( it != m_headers.end() )
		{
			m_state_context = it->second.template get<size_t>();
			parse_length();
		}
		else if( m_version == string_pool::v_1_1 )
		{
			it = m_headers.find(basic_header<CharT>::transfer_encoding);
			if( it == m_headers.end() or it->second.to_string() != string_pool::chunked )
				m_state = state::finished;
			else
			{
				m_state = state::chunked_wait_size;
				parse_chunked(error);
			}
		}
		else
			m_state = state::finished;
		return true;
	}

public:
#define LIBGS_HTTP_PARSER_ERRNO \
X_MACRO( PE_RLTL , 10000 , "Request line too long."      ) \
X_MACRO( PE_HLTL , 10001 , "Header line too long."       ) \
X_MACRO( PE_IRL  , 10002 , "Invalid request line."       ) \
X_MACRO( PE_IHM  , 10003 , "Invalid http method."        ) \
X_MACRO( PE_IHP  , 10004 , "Invalid http path."          ) \
X_MACRO( PE_IHL  , 10005 , "Invalid header line."        ) \
X_MACRO( PE_IDE  , 10006 , "The inserted data is empty." ) \
X_MACRO( PE_SFE  , 10007 , "Size format error."          ) \
X_MACRO( PE_RE   , 10008 , "This request is ended."      )

	enum parser_errno
	{
#define X_MACRO(e,v,d) e=(v),
		LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
	};
	class error_category : public std::error_category
	{
		LIBGS_DISABLE_COPY_MOVE(error_category)

	public:
		error_category() = default;
		[[nodiscard]] const char *name() const noexcept override {
			return "libgs::http::request_parser_error";
		}
		[[nodiscard]] std::string message(int code) const override
		{
			switch(code)
			{
#define X_MACRO(e,v,d) case e: return d;
				LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
				default: break;
			}
			return "Unknown error.";
		}
	};
	inline static error_category s_error_category;

#undef LIBGS_HTTP_PARSER_ERRNO
public:
	enum class state
	{
		waiting_request,      // GET /path HTTP/1.1\r\n
		reading_headers,      // Key: Value\r\n
		reading_length,       // Fixed length (Content-Length: 9\r\n).
		chunked_wait_size,    // 9\r\n
		chunked_wait_content, // body\r\n
		chunked_wait_headers, // Key: Value\r\n
		finished
	}
	m_state = state::waiting_request;
	size_t m_state_context = 0;
	std::string m_src_buf;

	http::method m_method = http::method::GET;
	string_t m_path;
	parameters_t m_parameters;
	path_args_t m_path_args;
	string_t m_version;

	headers_t m_headers;
	cookies_t m_cookies;
	std::string m_partial_body;

	bool m_keep_alive = true;
	bool m_support_gzip = false;
};

template <concept_char_type CharT>
basic_request_parser<CharT>::basic_request_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <concept_char_type CharT>
basic_request_parser<CharT>::~basic_request_parser()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_request_parser<CharT>::basic_request_parser(basic_request_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <concept_char_type CharT>
basic_request_parser<CharT> &basic_request_parser<CharT>::operator=(basic_request_parser &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf, error_code &error)
{
	using state = impl::state;
	error = error_code();
	if( buf.empty() )
	{
		error = error_code(impl::PE_IDE, impl::s_error_category);
		return false;
	}
	else if( m_impl->m_state == state::finished )
	{
		error = error_code(impl::PE_RE, impl::s_error_category);
		return false;
	}
	m_impl->m_src_buf.append(buf);
	if( m_impl->m_state <= state::reading_headers )
		return m_impl->parse_header(error);

	else if( m_impl->m_state == state::reading_length )
	{
		m_impl->parse_length();
		return true;
	}
	return m_impl->parse_chunked(error);
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf)
{
	error_code error;
	bool res = append(buf, error);
	if( error )
		throw system_error(error, "libgs::http::request_parser");
	return res;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::operator<<(std::string_view buf)
{
	return append(buf);
}

template <concept_char_type CharT>
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

template <concept_char_type CharT>
http::method basic_request_parser<CharT>::method() const noexcept
{
	return m_impl->m_method;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_request_parser<CharT>::path() const noexcept
{
	return m_impl->m_path;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_request_parser<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::parameters_t &basic_request_parser<CharT>::parameters() const noexcept
{
	return m_impl->m_parameters;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::path_args_t &basic_request_parser<CharT>::path_args() const noexcept
{
	return m_impl->m_path_args;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::headers_t &basic_request_parser<CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::cookies_t &basic_request_parser<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::can_read_from_device() const noexcept
{
	return m_impl->m_state > impl::state::reading_headers and
		   m_impl->m_state < impl::state::finished;
}

template <concept_char_type CharT>
std::string basic_request_parser<CharT>::take_partial_body(size_t size)
{
	if( size == 0 )
		return {};
	else if( size > m_impl->m_partial_body.size() )
		size = m_impl->m_partial_body.size();

	auto res = m_impl->m_partial_body.substr(0,size);
	m_impl->m_partial_body.erase(0,size);
	return res;
}

template <concept_char_type CharT>
std::string basic_request_parser<CharT>::take_body()
{
	return std::move(m_impl->m_partial_body);
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::is_finished() const noexcept
{
	return m_impl->m_state == impl::state::finished;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::is_eof() const noexcept
{
	return m_impl->m_partial_body.empty() and not can_read_from_device();
}

template <concept_char_type CharT>
basic_request_parser<CharT> &basic_request_parser<CharT>::reset()
{
	m_impl->reset();
	return *this;
}

} //namespace libgs::http

#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H
