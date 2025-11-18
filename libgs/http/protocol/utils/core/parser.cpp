
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

namespace libgs::http::protocol
{

static class LIBGS_DECL_HIDDEN error_category : public std::error_category
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
}
g_error_category;

[[nodiscard]] static error_code make_error_code(parse_errno errc) {
	return { static_cast<int>(errc), g_error_category };
}

class LIBGS_DECL_HIDDEN parser<model::base>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(size_t init_buf_size) {
		m_src_buf.reserve(init_buf_size);
	}

public:
	[[nodiscard]] sys_expected<bool> parse_header()
	{
		sys_expected<bool> result = false;
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() < 8192 )
					break;
				else if( m_state == state::waiting_request )
					result.despair(make_error_code(parse_errno::RLTL));
				else if( m_state == state::reading_headers )
					result.despair(make_error_code(parse_errno::HLTL));
				break;
			}
			auto line_buf = m_src_buf.substr(0, pos);
			m_src_buf.erase(0, pos + 2);

			if( m_state == state::waiting_request )
			{
				if( not m_parse_begin )
				{
					throw runtime_error (
						"libgs::http::parser: state_handler_waiting_begin == NULL."
					);
				}
				m_parse_begin(line_buf)
				.transform([&](version_enum version)
				{
					m_version = version;
					m_state = state::reading_headers;
				})
				.or_else([&](const error_code &error)
				{
					result.despair(error);
					reset();
				});
			}
			else if( m_state == state::reading_headers )
			{
				result = *state_handler_reading_headers(line_buf);
				if( result and *result )
					break;
			}
		}
		while( not m_src_buf.empty() );
		return result;
	}

	[[nodiscard]] sys_expected<bool> state_handler_reading_headers(std::string_view line_buf)
	{
		sys_expected<bool> result = false;
		if( line_buf.empty() )
		{
			set_read_body_state()
			.and_then([&]{
				result = true;
			})
			.or_else([&](const error_code &error) {
				result.despair(error);
			});
			return result;
		}
		auto colon_index = line_buf.find(':');
		if( colon_index == std::string::npos )
		{
			reset();
			return result.despair (
				make_error_code(parse_errno::IHL)
			);
		}
		header_insert (
			strtls::to_lower(strtls::trimmed(line_buf.substr(0, colon_index))),
			from_percent_encoding(strtls::trimmed(line_buf.substr(colon_index + 1)))
		)
		.or_else([&](const error_code &error)
		{
			result.despair(error);
			reset();
		});
		return result;
	}

	[[nodiscard]] error_code set_read_body_state()
	{
		error_code error;
		auto it = m_headers.find(header::content_length);
		if( it != m_headers.end() )
		{
			m_content_length = *it->second.get<size_t>().or_else();
			parse_length();
		}
		else if( m_version == version::v11 )
		{
			it = m_headers.find(header::transfer_encoding);
			if( it == m_headers.end() or it->second.to_string() != "chunked" )
				m_state = state::finished;
			else
			{
				m_state = state::chunked_wait_size;
				error = parse_chunked().error();
			}
		}
		else
			m_state = state::finished;
		return error;
	}

	void parse_length() noexcept
	{
		auto rsize = m_partial_body.size() + m_src_buf.size();
		rsize = rsize > m_content_length ? m_content_length - m_partial_body.size() : m_src_buf.size();

		m_partial_body += std::string(m_src_buf.c_str(), rsize);
		m_src_buf.clear();

		m_state = m_content_length > m_partial_body.size() ?
			state::reading_length : state::finished;
	}

	sys_expected<bool> parse_chunked()
	{
		sys_expected<bool> result = false;
		std::size_t _size = 0;
		do {
			auto pos = m_src_buf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( m_src_buf.size() > 8192 )
					result.despair(make_error_code(parse_errno::HLTL));
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
					result.despair(make_error_code(parse_errno::SFE));
					break;
				}
				try {
					_size = *strtls::to_arith<size_t>(line_buf, 16).or_else();
				}
				catch(...) {
					result.despair(make_error_code(parse_errno::SFE));
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
					result = true;
					break;
				}
				auto colon_index = line_buf.find(':');
				if( colon_index == std::string::npos )
				{
					result.despair(make_error_code(parse_errno::SFE));
					break;
				}
				header_insert (
					strtls::to_lower(strtls::trimmed(line_buf.substr(0, colon_index))),
					from_percent_encoding(strtls::trimmed(line_buf.substr(colon_index + 1)))
				)
				.or_else([&](const error_code &error)
				{
					result.despair(error);
					reset();
				});
			}
		}
		while( not m_src_buf.empty() );
		return result;
	}

	[[nodiscard]] error_code header_insert(std::string key, std::string value)
	{
		if( key == "cookie" or key == "set-cookie" )
		{
			if( not m_parse_cookie )
			{
				throw runtime_error (
					"libgs::http::parser: state_handler_waiting_begin == NULL."
				);
			}
			return m_parse_cookie(value);
		}
		m_headers[std::move(key)] = std::move(value);
		return {};
	}

	void reset()
	{
		m_state = state::waiting_request;
		m_version = static_cast<version_enum>(0);
		m_src_buf.clear();
		m_headers.clear();
		m_partial_body.clear();
	}

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

	version_enum m_version = static_cast<version_enum>(0);
	headers_t m_headers;

	std::string m_partial_body;
	size_t m_content_length = 0;

	parse_begin_handler m_parse_begin;
	parse_cookie_handler m_parse_cookie;
};

parser<model::base>::parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

parser<model::base>::~parser()
{
	delete m_impl;
}

parser<model::base>::parser(parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

parser<model::base> &parser<model::base>::operator=(parser &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

parser<model::base> &parser<model::base>::on_parse_begin(parse_begin_handler func)
{
	m_impl->m_parse_begin = std::move(func);
	return *this;
}

parser<model::base> &parser<model::base>::on_parse_cookie(parse_cookie_handler func)
{
	m_impl->m_parse_cookie = std::move(func);
	return *this;
}

error_code parser<model::base>::make_error_code(parse_errno errc)
{
	return protocol::make_error_code(errc);
}

sys_expected<bool> parser<model::base>::append(const const_buffer &buf)
{
	using state = impl::state;
	std::string str_buf(reinterpret_cast<const char*>(buf.data()), buf.size());

	if( str_buf.empty() )
		return { make_error_code(parse_errno::IDE) };

	else if( m_impl->m_state == state::finished )
		return { make_error_code(parse_errno::RE) };

	m_impl->m_src_buf += str_buf;
	if( m_impl->m_state <= state::reading_headers )
		return m_impl->parse_header();

	else if( m_impl->m_state == state::reading_length )
	{
		m_impl->parse_length();
		return true;
	}
	return m_impl->parse_chunked();
}

parser<model::base> &parser<model::base>::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

parser<model::base> &parser<model::base>::reset()
{
	m_impl->reset();
	return *this;
}

const headers &parser<model::base>::headers() const noexcept
{
	return m_impl->m_headers;
}

std::string parser<model::base>::take_partial_body(size_t size)
{
	if( size == 0 )
		return {};
	else if( size > m_impl->m_partial_body.size() )
		size = m_impl->m_partial_body.size();

	auto res = m_impl->m_partial_body.substr(0,size);
	m_impl->m_partial_body.erase(0,size);
	return res;
}

std::string parser<model::base>::take_body()
{
	return std::move(m_impl->m_partial_body);
}

version_enum parser<model::base>::version() const noexcept
{
	return m_impl->m_version;
}

bool parser<model::base>::can_read_from_device() const noexcept
{
	return m_impl->m_state > impl::state::reading_headers and
		   m_impl->m_state < impl::state::finished;
}

bool parser<model::base>::is_finished() const noexcept
{
	return m_impl->m_state == impl::state::finished;
}

bool parser<model::base>::is_eof() const noexcept
{
	return m_impl->m_partial_body.empty() and not can_read_from_device();
}

parser<model::base> &parser<model::base>::unbind_parse_begin()
{
	m_impl->m_parse_begin = {};
	return *this;
}

parser<model::base> &parser<model::base>::unbind_parse_cookie()
{
	m_impl->m_parse_cookie = {};
	return *this;
}

} //namespace libgs::http::protocol