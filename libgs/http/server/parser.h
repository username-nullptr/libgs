
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

#ifndef LIBGS_HTTP_SERVER_PARSER_H
#define LIBGS_HTTP_SERVER_PARSER_H

#include <libgs/http/server/datagram.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_server_parser
{
	LIBGS_DISABLE_COPY(basic_server_parser)

public:
	explicit basic_server_parser(size_t init_buf_size = 0xFFFF);
	~basic_server_parser();

	basic_server_parser(basic_server_parser &&other) noexcept;
	basic_server_parser &operator=(basic_server_parser &&other) noexcept;

public:
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);

public:
	using datagram_type = basic_server_datagram<CharT>;
	datagram_type get_result();

protected:
	class impl;
	impl *m_impl;
};

using server_parser = basic_server_parser<char>;

using server_wparser = basic_server_parser<wchar_t>;

} //namespace libgs::http


#include <libgs/core/string_list.h>

#include <libgs/core/log.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_server_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(std::size_t init_buf_size)
	{
		m_buffer.reserve(init_buf_size);
	}

public:
	void state_handler_waiting_request(std::string_view line_buf)
	{
		auto request_line_parts = string_list::from_string(line_buf, ' ');
		if( request_line_parts.size() != 3 or not str_to_upper(request_line_parts[2]).starts_with("HTTP/") )
		{
			m_buffer.clear();
			throw runtime_error("libgs::http::server::parser: Invalid request line.");
		}
		http::method method;
		try {
			method = from_method_string(request_line_parts[0]);
		}
		catch(std::exception&)
		{
			m_buffer.clear();
			throw runtime_error("libgs::http::server::parser: Invalid http method.");
		}
		m_datagram.method  = method;
		m_datagram.version = mbstoxx<CharT>(request_line_parts[2].substr(5,3));

		auto resource_line = from_percent_encoding(request_line_parts[1]);
		auto pos = resource_line.find('?');

		if( pos == std::string::npos )
			m_datagram.path = mbstoxx<CharT>(resource_line);
		else
		{
			m_datagram.path = mbstoxx<CharT>(resource_line.substr(0, pos));
			auto parameters_string = resource_line.substr(pos + 1);

			for(auto &para_str : string_list::from_string(parameters_string, '&'))
			{
				pos = para_str.find('=');
				if( pos == std::string::npos )
					m_datagram.parameters.emplace(mbstoxx<CharT>(para_str), mbstoxx<CharT>(para_str));
				else
					m_datagram.parameters.emplace(mbstoxx<CharT>(para_str.substr(0, pos)), mbstoxx<CharT>(para_str.substr(pos+1)));
			}
		}
		auto n_it = std::unique(m_datagram.path.begin(), m_datagram.path.end(), [](CharT c0, CharT c1){
			return c0 == c1 and c0 == 0x2F/*/*/;
		});
		if( n_it != m_datagram.path.end() )
			m_datagram.path.erase(n_it, m_datagram.path.end());
		m_state = state::reading_headers;
	}

	[[nodiscard]] bool state_handler_reading_headers(std::string_view line_buf)
	{
		if( line_buf.empty() )
			return next_request_ready();

		auto colon_index = line_buf.find(':');
		if( colon_index == std::string::npos )
		{
			reset();
			throw runtime_error("libgs::http::server::parser: Invalid header line.");
		}
		auto key = str_to_lower(str_trimmed(line_buf.substr(0, colon_index)));
		auto value = from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1)));

		if( key != "cookie" )
		{
			m_datagram.headers[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
			return false;
		}
		auto list = string_list::from_string(value, ';');
		for(auto &statement : list)
		{
			statement = str_trimmed(statement);
			auto pos = statement.find('=');

			if( pos == std::string::npos )
			{
				reset();
				throw runtime_error("libgs::http::server::parser: Invalid header line.");
			}
			key = str_trimmed(statement.substr(0,pos));
			value = str_trimmed(statement.substr(pos+1));
			m_datagram.cookies[mbstoxx<CharT>(key)] = mbstoxx<CharT>(value);
		}
		return false;
	}

public:
	bool next_request_ready()
	{
		m_state = state::waiting_request;
		return true;
	}

	void reset()
	{
		m_state = state::waiting_request;
		m_buffer.clear();
		datagram_clear();
	}

	void datagram_clear()
	{
		m_datagram.version.clear();
		m_datagram.headers.clear();
		m_datagram.partial_body.clear();

		m_datagram.path.clear();
		m_datagram.parameters.clear();
		m_datagram.cookies.clear();
	}

public:
	enum class state
	{
		waiting_request = 0,
		reading_headers
	}
	m_state = state::waiting_request;
	std::string m_buffer;
	datagram_type m_datagram;
};

template <concept_char_type CharT>
basic_server_parser<CharT>::basic_server_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <concept_char_type CharT>
basic_server_parser<CharT>::~basic_server_parser()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_server_parser<CharT>::basic_server_parser(basic_server_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <concept_char_type CharT>
basic_server_parser<CharT> &basic_server_parser<CharT>::operator=(basic_server_parser &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
}

template <concept_char_type CharT>
bool basic_server_parser<CharT>::append(std::string_view buf)
{
	using state = impl::state;
	m_impl->m_buffer.append(buf);

	while( not m_impl->m_buffer.empty() )
	{
		auto pos = m_impl->m_buffer.find("\r\n");
		if( pos == std::string::npos )
		{
			if( m_impl->m_buffer.size() < 1024 )
				return false;
			else if( m_impl->m_state == state::waiting_request )
			{
				m_impl->m_buffer.clear();
				throw runtime_error("libgs::http::server::parser: Request line too long.");
			}
			else if( m_impl->m_state == state::reading_headers )
			{
				m_impl->reset();
				throw runtime_error("libgs::http::server::parser: Header line too long.");
			}
			return false;
		}
		auto line_buf = m_impl->m_buffer.substr(0, pos);
		m_impl->m_buffer.erase(0, pos + 2);

		if( m_impl->m_state == state::waiting_request )
			m_impl->state_handler_waiting_request(line_buf);

		else if( m_impl->m_state == state::reading_headers )
		{
			if( m_impl->state_handler_reading_headers(line_buf) )
				return true;
		}
	}
	return false;
}

template <concept_char_type CharT>
bool basic_server_parser<CharT>::operator<<(std::string_view buf)
{
	return append(buf);
}

template <concept_char_type CharT>
basic_server_datagram<CharT> basic_server_parser<CharT>::get_result()
{
	return std::move(m_impl->m_datagram);
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_PARSER_H
