
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_PARSER_H
#define LIBGS_HTTP_BASIC_DETAIL_PARSER_H

#include <libgs/core/string_list.h>

namespace libgs::http
{

namespace detail
{

template <concept_char_type T>
struct _parser_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *comma = __VA_ARGS__##",";

template <>
struct _parser_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _parser_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	struct string_pool : detail::string_pool<CharT>, detail::_parser_static_string<CharT> {};

public:
	impl(size_t init_buf_size) {
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
					error = make_error_code(parse_errno::RLTL);
				else if( m_state == state::reading_headers )
					error = make_error_code(parse_errno::HLTL);
				break;
			}
			auto line_buf = m_src_buf.substr(0, pos);
			m_src_buf.erase(0, pos + 2);

			if( m_state == state::waiting_request )
			{
				if( not m_parse_begin )
					throw runtime_error("libgs::http::parser: state_handler_waiting_begin == NULL.");

				m_version = m_parse_begin(line_buf, error);
				if( error )
				{
					m_src_buf.clear();
					break;
				}
				m_state = state::reading_headers;
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

	[[nodiscard]] bool state_handler_reading_headers(std::string_view line_buf, error_code &error)
	{
		if( line_buf.empty() )
			return set_read_body_state(error);

		auto colon_index = line_buf.find(':');
		if( colon_index == std::string::npos )
		{
			reset();
			error = make_error_code(parse_errno::IHL);
			return false;
		}
		header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
					  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))), error);
		return false;
	}

	bool set_read_body_state(error_code &error)
	{
		auto it = m_headers.find(basic_header<CharT>::content_length);
		if( it != m_headers.end() )
		{
			m_content_length = it->second.template get<size_t>();
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

	void parse_length() noexcept
	{
		auto rsize = m_partial_body.size() + m_src_buf.size();
		rsize = rsize > m_content_length ? m_content_length - m_partial_body.size() : m_src_buf.size();

		m_partial_body += std::string(m_src_buf.c_str(), rsize);
		m_src_buf.clear();

		m_state = m_content_length > m_partial_body.size() ? state::reading_length : state::finished;
	}

	bool parse_chunked(error_code &error)
	{
		std::size_t _size = 0;
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() >= 1024 )
					error = make_error_code(parse_errno::HLTL);
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
					error = make_error_code(parse_errno::SFE);
					break;
				}
				try {
					_size = ston<size_t>(line_buf, 16);
				}
				catch(...) {
					error = make_error_code(parse_errno::SFE);
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
					error = make_error_code(parse_errno::SFE);
					break;
				}
				header_insert(str_to_lower(str_trimmed(line_buf.substr(0, colon_index))),
							  from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1))), error);
			}
		}
		while( not m_src_buf.empty() );
		return false;
	}

	void header_insert(std::string key, std::string value, error_code &error)
	{
		if( key == "cookie" or key == "set-cookie" )
		{
			if( not m_parse_cookie )
				throw runtime_error("libgs::http::parser: state_handler_waiting_begin == NULL.");
			m_parse_cookie(value, error);
		}
		else
			m_headers[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
	}

	void reset()
	{
		m_state = state::waiting_request;
		m_src_buf.clear();
		m_headers.clear();
		m_partial_body.clear();
	}

public:
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
			switch(static_cast<parse_errno>(code))
			{
#define X_MACRO(e,v,d) case parse_errno::e: return d;
				LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
				default: break;
			}
			return "Unknown error.";
		}
	};
	inline static error_category s_error_category;

	static error_code make_error_code(parse_errno errc) {
		return error_code(static_cast<int>(errc), s_error_category);
	}

#undef LIBGS_HTTP_PARSER_ERRNO
public:
	enum class state
	{
		waiting_request,      // GET /path HTTP/1.1\r\n
		                      // HTTP/1.1 200 OK\r\n
		reading_headers,      // Key: Value\r\n
		reading_length,       // Fixed length (Content-Length: 9\r\n).
		chunked_wait_size,    // 9\r\n
		chunked_wait_content, // body\r\n
		chunked_wait_headers, // Key: Value\r\n
		finished
	}
	m_state = state::waiting_request;
	std::string m_src_buf;

	string_t m_version;
	headers_t m_headers;

	std::string m_partial_body;
	size_t m_content_length = 0;

	parse_begin_handler m_parse_begin;
	parse_cookie_handler m_parse_cookie;
};

template <concept_char_type CharT>
basic_parser<CharT>::basic_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <concept_char_type CharT>
basic_parser<CharT>::~basic_parser()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_parser<CharT>::basic_parser(basic_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::operator=(basic_parser &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::on_parse_begin(parse_begin_handler func)
{
	m_impl->m_parse_begin = std::move(func);
	return *this;
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::on_parse_cookie(parse_cookie_handler func)
{
	m_impl->m_parse_cookie = std::move(func);
	return *this;
}

template <concept_char_type CharT>
error_code basic_parser<CharT>::make_error_code(parse_errno errc)
{
	return impl::make_error_code(errc);
}

template <concept_char_type CharT>
bool basic_parser<CharT>::append(std::string_view buf, error_code &error)
{
	using state = impl::state;
	error = error_code();
	if( buf.empty() )
	{
		error = make_error_code(parse_errno::IDE);
		return false;
	}
	else if( m_impl->m_state == state::finished )
	{
		error = make_error_code(parse_errno::RE);
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
bool basic_parser<CharT>::append(std::string_view buf)
{
	error_code error;
	bool res = append(buf, error);
	if( error )
		throw system_error(error, "libgs::http::parser");
	return res;
}

template <concept_char_type CharT>
bool basic_parser<CharT>::operator<<(std::string_view buf)
{
	return append(buf);
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::reset()
{
	m_impl->reset();
	return *this;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_parser<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <concept_char_type CharT>
const typename basic_parser<CharT>::headers_t &basic_parser<CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <concept_char_type CharT>
std::string basic_parser<CharT>::take_partial_body(size_t size)
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
std::string basic_parser<CharT>::take_body()
{
	return std::move(m_impl->m_partial_body);
}

template <concept_char_type CharT>
bool basic_parser<CharT>::can_read_from_device() const noexcept
{
	return m_impl->m_state > impl::state::reading_headers and
		   m_impl->m_state < impl::state::finished;
}

template <concept_char_type CharT>
bool basic_parser<CharT>::is_finished() const noexcept
{
	return m_impl->m_state == impl::state::finished;
}

template <concept_char_type CharT>
bool basic_parser<CharT>::is_eof() const noexcept
{
	return m_impl->m_partial_body.empty() and not can_read_from_device();
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::unset_parse_begin()
{
	m_impl->m_parse_begin = {};
	return *this;
}

template <concept_char_type CharT>
basic_parser<CharT> &basic_parser<CharT>::unset_parse_cookie()
{
	m_impl->m_parse_cookie = {};
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_DETAIL_PARSER_H
