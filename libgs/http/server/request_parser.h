
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_PARSER_H
#define LIBGS_HTTP_SERVER_REQUEST_PARSER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser
{
	LIBGS_DISABLE_COPY(basic_request_parser)

public:
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using cookies_type = basic_cookie_values<CharT>;
	using header_type = basic_header<CharT>;
	using headers_type = basic_headers<CharT>;
	using parameters_type = basic_parameters<CharT>;

public:
	explicit basic_request_parser(size_t init_buf_size = 0xFFFF);
	~basic_request_parser();

	basic_request_parser(basic_request_parser &&other) noexcept;
	basic_request_parser &operator=(basic_request_parser &&other) noexcept;

public:
	bool append(std::string_view buf, error_code &error);
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_view_type path() const noexcept;
	[[nodiscard]] str_view_type version() const noexcept;

public:
	[[nodiscard]] const parameters_type &parameters() const noexcept;
	[[nodiscard]] const headers_type &headers() const noexcept;
	[[nodiscard]] const cookies_type &cookies() const noexcept;

public:
	[[nodiscard]] bool is_websocket_handshake() const;
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool can_read_body() const;

public:
	[[nodiscard]] std::string take_partial_body(size_t size = 0);
	[[nodiscard]] bool is_finished() const noexcept;
	basic_request_parser &reset();

private:
	class impl;
	impl *m_impl;
};

using request_parser = basic_request_parser<char>;
using wrequest_parser = basic_request_parser<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/request_parser.h>


#include <libgs/core/string_list.h>

namespace libgs::http
{

namespace detail
{

template <typename T>
struct _key_static_string;

#define LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(_type, ...) \
	static constexpr const _type *v_1_0      = __VA_ARGS__##"1.0"       ; \
	static constexpr const _type *v_1_1      = __VA_ARGS__##"1.1"       ; \
	static constexpr const _type *close      = __VA_ARGS__##"close"     ; \
	static constexpr const _type *comma      = __VA_ARGS__##","         ; \
	static constexpr const _type *gzip       = __VA_ARGS__##"gzip"      ; \
	static constexpr const _type *chunked    = __VA_ARGS__##"chunked"   ; \
	static constexpr const _type *upgrade    = __VA_ARGS__##"upgrade"   ; \
	static constexpr const _type *websocket  = __VA_ARGS__##"websocket" ;

template <>
struct _key_static_string<char> {
	LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(char);
};

template <>
struct _key_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(wchar_t,L);
};

} //namespace detail

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using static_string = detail::_key_static_string<CharT>;

public:
	using str_type = std::basic_string<CharT>;
	using headers_type = basic_headers<CharT>;

	using cookies_type = std::map<std::basic_string<CharT>, basic_value<CharT>, less_case_insensitive>;
	using parameters_type = basic_parameters<CharT>;

public:
	explicit impl(size_t init_buf_size)
	{
		m_src_buf.reserve(init_buf_size);
	}

public:
	[[nodiscard]] bool parse_header()
	{
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() < 1024 )
					return false;

				else if( m_state == state::waiting_request )
					error("request line too long");

				else if( m_state == state::reading_headers )
					error("header line too long");

				return false;
			}
			auto line_buf = m_src_buf.substr(0, pos);
			m_src_buf.erase(0, pos + 2);

			if( m_state == state::waiting_request )
				state_handler_waiting_request(line_buf);

			else if( m_state == state::reading_headers )
			{
				if( state_handler_reading_headers(line_buf) )
					return true;
			}
		}
		while( not m_src_buf.empty() );
		return false;
	}

	bool parse_length() noexcept
	{
		auto tsize = m_state_context;
		auto rsize = m_partial_body.size() + m_src_buf.size();
		rsize = rsize > tsize ? tsize - m_partial_body.size() : m_src_buf.size();

		m_partial_body += std::string(m_src_buf.c_str(), rsize);
		m_src_buf.clear();

		m_state = tsize > m_partial_body.size() ? state::reading_length : state::finished;
		return true;
	}

	bool parse_chunked()
	{
		std::size_t _size = 0;
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() >= 1024 )
					error("header line too long");
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
					error("size format error");

				try {
					_size = ston<size_t>(line_buf, 16);
				}
				catch(...) {
					error("size format error");
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
					error("size format error");

				header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
							  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))));
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
	void state_handler_waiting_request(std::string_view line_buf)
	{
		auto request_line_parts = string_list::from_string(line_buf, ' ');
		if( request_line_parts.size() != 3 or not str_to_upper(request_line_parts[2]).starts_with("HTTP/") )
		{
			m_src_buf.clear();
			throw runtime_error("libgs::http::server::parser: Invalid request line.");
		}
		http::method method;
		try {
			method = from_method_string(request_line_parts[0]);
		}
		catch(std::exception&)
		{
			m_src_buf.clear();
			throw runtime_error("libgs::http::server::parser: Invalid http method.");
		}
		m_method  = method;
		m_version = mbstoxx<CharT>(request_line_parts[2].substr(5,3));

		auto resource_line = from_percent_encoding(request_line_parts[1]);
		auto pos = resource_line.find('?');

		if( pos == std::string::npos )
			m_path = mbstoxx<CharT>(resource_line);
		else
		{
			m_path = mbstoxx<CharT>(resource_line.substr(0, pos));
			auto parameters_string = resource_line.substr(pos + 1);

			for(auto &para_str : string_list::from_string(parameters_string, '&'))
			{
				pos = para_str.find('=');
				if( pos == std::string::npos )
					m_parameters.emplace(mbstoxx<CharT>(para_str), mbstoxx<CharT>(para_str));
				else
					m_parameters.emplace(mbstoxx<CharT>(para_str.substr(0, pos)), mbstoxx<CharT>(para_str.substr(pos+1)));
			}
		}
		auto n_it = std::unique(m_path.begin(), m_path.end(), [](CharT c0, CharT c1){
			return c0 == c1 and c0 == 0x2F/*/*/;
		});
		if( n_it != m_path.end() )
			m_path.erase(n_it, m_path.end());
		m_state = state::reading_headers;
	}

	[[nodiscard]] bool state_handler_reading_headers(std::string_view line_buf)
	{
		if( line_buf.empty() )
			return set_read_body_state();

		auto colon_index = line_buf.find(':');
		if( colon_index == std::string::npos )
		{
			reset();
			throw runtime_error("libgs::http::server::parser: Invalid header line.");
		}
		header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
					  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))));
		return false;
	}

private:
	void header_insert(std::string key, std::string value)
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
				error("invalid header line");

			key = str_trimmed(statement.substr(0,pos));
			value = str_trimmed(statement.substr(pos+1));
			m_cookies[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
		}
	}

	bool set_read_body_state()
	{
		auto it = m_headers.find(basic_header<CharT>::connection);
		if( it == m_headers.end() )
			m_keep_alive = m_version != static_string::v_1_0;
		else
			m_keep_alive = str_to_lower(it->second.to_string()) != static_string::close;

		it = m_headers.find(basic_header<CharT>::accept_encoding);
		if( it == m_headers.end() )
			m_support_gzip = false;
		else
		{
			for(auto &str : basic_string_list<CharT>::from_string(it->second.to_string(), static_string::comma))
			{
				if( str_to_lower(str_trimmed(str)) == static_string::gzip )
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
		else if( m_version == static_string::v_1_1 )
		{
			it = m_headers.find(basic_header<CharT>::transfer_encoding);
			if( it == m_headers.end() or it->second.to_string() != static_string::chunked )
				m_state = state::finished;
			else
			{
				m_state = state::chunked_wait_size;
				parse_chunked();
			}
		}
		else
			m_state = state::finished;

		if( m_version == static_string::v_1_1 )
		{
			it = m_headers.find(basic_header<CharT>::connection);
			if( it != m_headers.end() and str_to_lower(it->second.to_string()) == static_string::upgrade )
			{
				it = m_headers.find(basic_header<CharT>::upgrade);
				if( it != m_headers.end() and str_to_lower(it->second.to_string()) == static_string::websocket )
					m_websocket = true;
			}
		}
		return true;
	}

	void error(const char *msg)
	{
		reset();
		throw runtime_error("libgs::http::server::parser: {}.", msg);
	}

public:
	enum class state
	{
		waiting_request,
		reading_headers,
		reading_length,
		chunked_wait_size,
		chunked_wait_content,
		chunked_wait_headers,
		finished
	}
	m_state = state::waiting_request;
	size_t m_state_context = 0;
	std::string m_src_buf;

	http::method m_method = http::method::GET;
	str_type m_path;
	parameters_type m_parameters;
	str_type m_version;

	headers_type m_headers;
	cookies_type m_cookies;
	std::string m_partial_body;

	bool m_keep_alive = true;
	bool m_support_gzip = false;
	bool m_websocket = false;
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
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf, error_code &error)
{
	bool res = false;
	try {
		error = error_code();
		res = append(buf);
	}
	catch(std::exception &ex)
	{
		class error_category : public std::error_category
		{
		public:
			error_category(const char *what) : m_what(what) {}

			[[nodiscard]] const char *name() const noexcept override {
				return "libgs::http::request_parser_error";
			}
			[[nodiscard]] std::string message(int) const override {
				return m_what;
			}

		private:
			const char *m_what;
		};
		error = error_code(-1, error_category(ex.what()));
	}
	return res;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::append(std::string_view buf)
{
	using state = impl::state;
	if( buf.empty() )
		throw runtime_error("libgs::http::server::parser: the inserted data is empty.");

	else if( m_impl->m_state == state::finished )
		throw runtime_error("libgs::http::server::parser: this request is ended.");

	m_impl->m_src_buf.append(buf);
	if( m_impl->m_state <= state::reading_headers )
		return m_impl->parse_header();

	else if( m_impl->m_state == state::reading_length )
		return m_impl->parse_length();

	return m_impl->parse_chunked();
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::operator<<(std::string_view buf)
{
	return append(buf);
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
const typename basic_request_parser<CharT>::parameters_type &basic_request_parser<CharT>::parameters() const noexcept
{
	return m_impl->m_parameters;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::headers_type &basic_request_parser<CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <concept_char_type CharT>
const typename basic_request_parser<CharT>::cookies_type &basic_request_parser<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::is_websocket_handshake() const
{
	return m_impl->m_websocket;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::keep_alive() const
{
	return m_impl->m_keep_alive;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::support_gzip() const
{
	return m_impl->m_support_gzip;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::can_read_body() const
{
	return m_impl->m_state > impl::state::reading_headers and
		   m_impl->m_state < impl::state::finished;
}

template <concept_char_type CharT>
std::string basic_request_parser<CharT>::take_partial_body(size_t size)
{
	if( size == 0 )
		return std::move(m_impl->m_partial_body);

	else if( size < m_impl->m_partial_body.size() )
		size = m_impl->m_partial_body.size();

	auto res = m_impl->m_partial_body.substr(0,size);
	m_impl->m_partial_body.erase(0,size);
	return res;
}

template <concept_char_type CharT>
bool basic_request_parser<CharT>::is_finished() const noexcept
{
	return m_impl->m_state == impl::state::finished;
}

template <concept_char_type CharT>
basic_request_parser<CharT> &basic_request_parser<CharT>::reset()
{
	m_impl->reset();
	return *this;
}

} //namespace libgs::http



#endif //LIBGS_HTTP_SERVER_REQUEST_PARSER_H
