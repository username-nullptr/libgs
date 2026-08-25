
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
#include <libgs/core/algorithm/misc.h>
#include <libgs/core/string_vector.h>

namespace libgs::http_nt { namespace
{

class LIBGS_DECL_HIDDEN error_category : public std::error_category
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

} //namespace

[[nodiscard]] static error_code make_error_code(parse_errno errc) {
	return { static_cast<int>(errc), g_error_category };
}

class LIBGS_DECL_HIDDEN parser<protocol_model::base>::impl
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
					runtime_error::loc_throw (
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
				auto expected = state_handler_reading_headers(line_buf);
				if( not expected )
				{
					result.despair(expected.error());
					break;
				}
				result = *expected;
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
			if( auto error = set_read_body_state() )
				result.despair(error);
			else
				result = true;
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
		auto error = header_insert (
			strtls::to_lower(strtls::trimmed(line_buf.substr(0, colon_index))),
			strtls::trimmed(line_buf.substr(colon_index + 1))
		);
		if( error )
		{
			result.despair(error);
			reset();
		}
		return result;
	}

	[[nodiscard]] error_code set_read_body_state()
	{
		error_code error;
		if( m_skip_body )
		{
			m_state = state::finished;
			return error;
		}
		auto content_length = m_headers.find(header::content_length);
		auto transfer_encoding = m_headers.find(header::transfer_encoding);

		if( content_length != m_headers.end() and transfer_encoding != m_headers.end() )
			return make_error_code(parse_errno::SFE);

		if( content_length != m_headers.end() )
		{
			auto expected = content_length->second.get<size_t>();
			if( not expected )
				return make_error_code(parse_errno::SFE);

			m_content_length = *expected;
			parse_length();
		}
		else if( transfer_encoding != m_headers.end() )
		{
			if( m_version != version::v11 )
				return make_error_code(parse_errno::SFE);

			auto codings = string_vector::from_string(transfer_encoding->second.to_string(), ',');
			if( codings.size() != 1 or strtls::to_lower(strtls::trimmed(codings.back())) != "chunked" )
				return make_error_code(parse_errno::SFE);

			m_state = state::chunked_wait_size;
			parse_chunked().or_else([&](const error_code &e) {
				error = e;
			});
		}
		else if( m_read_until_eof )
		{
			m_state = state::reading_eof;
			if( not m_src_buf.empty() )
			{
				m_partial_body += m_src_buf;
				m_src_buf.clear();
			}
		}
		else
			m_state = state::finished;
		return error;
	}

	void parse_length() noexcept
	{
		auto rsize = m_content_length - m_content_length_counter;
		if( rsize > m_src_buf.size() )
			rsize = m_src_buf.size();

		m_content_length_counter += rsize;
		m_partial_body += m_src_buf.substr(0, rsize);
		m_src_buf.erase(0, rsize);

		m_state = m_content_length_counter == m_content_length ?
			state::finished : state::reading_length;
	}

	[[nodiscard]] static bool is_token_char(uint8_t ch) noexcept
	{
		constexpr std::string_view c_table = "!#$%&'*+-.^_`|~";
		return (ch >= '0' and ch <= '9') or
			   (ch >= 'A' and ch <= 'Z') or
			   (ch >= 'a' and ch <= 'z') or
			   c_table.find(static_cast<char>(ch)) != std::string_view::npos;
	}

	[[nodiscard]] static bool is_quoted_char(uint8_t ch) noexcept
	{
		return ch == '\t' or ch == ' ' or ch == 0x21 or
			(ch >= 0x23 and ch <= 0x5B) or
			(ch >= 0x5D and ch <= 0x7E) or ch >= 0x80;
	}

	[[nodiscard]] error_code parse_chunk_attributes(std::string_view line_buf)
	{
		chunk_attributes_t attributes {};
		size_t pos = 0;
		auto skip_bws = [&]() noexcept
		{
			while( pos < line_buf.size() and (line_buf[pos] == ' ' or line_buf[pos] == '\t') )
				++pos;
		};
		skip_bws();
		while( pos < line_buf.size() )
		{
			if( line_buf[pos] != ';' )
				return make_error_code(parse_errno::SFE);
			++pos;
			skip_bws();

			auto begin = pos;
			while( pos < line_buf.size() and is_token_char(static_cast<uint8_t>(line_buf[pos])) )
				++pos;

			if( begin == pos )
				return make_error_code(parse_errno::SFE);

			std::string attribute(line_buf.substr(begin, pos - begin));
			skip_bws();
			if( pos < line_buf.size() and line_buf[pos] == '=' )
			{
				attribute += '=';
				++pos;
				skip_bws();
				if( pos == line_buf.size() )
					return make_error_code(parse_errno::SFE);

				begin = pos;
				if( line_buf[pos] == '"' )
				{
					++pos;
					bool closed = false;
					while( pos < line_buf.size() )
					{
						auto ch = static_cast<uint8_t>(line_buf[pos++]);
						if( ch == '"' )
						{
							closed = true;
							break;
						}
						if( ch == '\\' )
						{
							if( pos == line_buf.size() )
								return make_error_code(parse_errno::SFE);

							ch = static_cast<uint8_t>(line_buf[pos++]);
							if( ch != '\t' and (ch < 0x20 or ch == 0x7F) )
								return make_error_code(parse_errno::SFE);
						}
						else if( not is_quoted_char(ch) )
							return make_error_code(parse_errno::SFE);
					}
					if( not closed )
						return make_error_code(parse_errno::SFE);
				}
				else
				{
					while( pos < line_buf.size() and is_token_char(static_cast<uint8_t>(line_buf[pos])) )
						++pos;

					if( begin == pos )
						return make_error_code(parse_errno::SFE);
				}
				attribute += line_buf.substr(begin, pos - begin);
				skip_bws();
			}
			if( pos < line_buf.size() and line_buf[pos] != ';' )
				return make_error_code(parse_errno::SFE);
			attributes.emplace(std::move(attribute));
		}
		for(auto &attribute : attributes)
			m_chunk_attributes.emplace(attribute);
		return {};
	}

	sys_expected<bool> parse_chunked()
	{
		sys_expected<bool> result = false;
		for(;;)
		{
			if( m_state == state::chunked_wait_size )
			{
				auto pos = m_src_buf.find("\r\n");
				if( pos == std::string::npos )
				{
					if( m_src_buf.size() > 8192 )
						result.despair(make_error_code(parse_errno::HLTL));
					return result;
				}

				auto line_buf = m_src_buf.substr(0, pos);
				m_src_buf.erase(0, pos + 2);
				auto attributes_pos = line_buf.find(';');
				auto size_buf = line_buf.substr(0, attributes_pos);

				size_buf = strtls::trimmed(size_buf);
				if( size_buf.empty() or size_buf.size() > sizeof(size_t) * 2 )
				{
					result.despair(make_error_code(parse_errno::SFE));
					return result;
				}
				auto expected = strtls::to_arith<size_t>(size_buf, 16);
				if( not expected )
				{
					result.despair(make_error_code(parse_errno::SFE));
					return result;
				}
				if( attributes_pos != std::string::npos )
				{
					auto error = parse_chunk_attributes (
						std::string_view(line_buf).substr(attributes_pos)
					);
					if( error )
					{
						result.despair(error);
						return result;
					}
				}
				m_chunk_size = *expected;
				m_state = m_chunk_size == 0 ?
					state::chunked_wait_headers : state::chunked_wait_content;
				continue;
			}
			if( m_state == state::chunked_wait_content )
			{
				if( m_src_buf.empty() )
					return result;

				auto size = std::min(m_chunk_size, m_src_buf.size());
				m_partial_body.append(m_src_buf, 0, size);
				m_src_buf.erase(0, size);
				m_chunk_size -= size;

				if( m_chunk_size == 0 )
					m_state = state::chunked_wait_content_end;
				continue;
			}
			if( m_state == state::chunked_wait_content_end )
			{
				if( m_src_buf.size() < 2 )
					return result;
				if( not m_src_buf.starts_with("\r\n") )
				{
					result.despair(make_error_code(parse_errno::SFE));
					return result;
				}
				m_src_buf.erase(0, 2);
				m_state = state::chunked_wait_size;
				continue;
			}
			if( m_state == state::chunked_wait_headers )
			{
				auto pos = m_src_buf.find("\r\n");
				if( pos == std::string::npos )
				{
					if( m_src_buf.size() > 8192 )
						result.despair(make_error_code(parse_errno::HLTL));
					return result;
				}
				auto line_buf = m_src_buf.substr(0, pos);
				m_src_buf.erase(0, pos + 2);

				if( line_buf.empty() )
				{
					m_state = state::finished;
					result = true;
					return result;
				}
				auto colon_index = line_buf.find(':');
				if( colon_index == std::string::npos )
				{
					result.despair(make_error_code(parse_errno::SFE));
					return result;
				}
				auto error = header_insert (
					strtls::to_lower(strtls::trimmed(line_buf.substr(0, colon_index))),
					strtls::trimmed(line_buf.substr(colon_index + 1))
				);
				if( error )
				{
					result.despair(error);
					reset();
					return result;
				}
			}
		}
	}

	[[nodiscard]] error_code header_insert(std::string key, std::string value)
	{
		if( key == "cookie" or key == "set-cookie" )
		{
			if( not m_parse_cookie )
			{
				runtime_error::loc_throw (
					"libgs::http::parser: state_handler_waiting_begin == NULL."
				);
			}
			return m_parse_cookie(value);
		}
		if( auto it = m_headers.find(key); it != m_headers.end() )
		{
			if( key == "content-length" )
				return make_error_code(parse_errno::SFE);
			it->second = it->second.to_string() + ", " + value;
		}
		else
			m_headers[std::move(key)] = std::move(value);
		return {};
	}

	void reset(bool preserve_input = false)
	{
		m_state = state::waiting_request;
		m_version = version::none;

		if( not preserve_input )
			m_src_buf.clear();

		m_headers.clear();
		m_chunk_attributes.clear();
		m_partial_body.clear();

		m_content_length_counter = 0;
		m_content_length = 0;
		m_chunk_size = 0;
		m_skip_body = false;
	}

public:
	enum class state
	{
		waiting_request,          // GET /path HTTP/1.1\r\n
							      // HTTP/1.1 200 OK\r\n
		reading_headers,          // Key: Value\r\n
		reading_length,           // Fixed length (Content-Length: 9\r\n).
		reading_eof,              // Response body delimited by connection close.
		chunked_wait_size,        // 9\r\n
		chunked_wait_content,     // body
		chunked_wait_content_end, // \r\n
		chunked_wait_headers,     // Key: Value\r\n
		finished
	}
	m_state = state::waiting_request;
	std::string m_src_buf {};

	version_enum m_version = static_cast<version_enum>(0);
	headers_t m_headers {};

	chunk_attributes_t m_chunk_attributes {};
	std::string m_partial_body {};

	size_t m_content_length_counter = 0;
	size_t m_content_length = 0;
	size_t m_chunk_size = 0;

	bool m_skip_body = false;
	bool m_read_until_eof = false;

	parse_begin_handler m_parse_begin {};
	parse_cookie_handler m_parse_cookie {};
};

parser<protocol_model::base>::parser(size_t init_buf_size) :
	const_headers(nullptr),
	m_impl(new impl(init_buf_size))
{
	m_headers = &m_impl->m_headers;
}

parser<protocol_model::base>::~parser()
{
	delete m_impl;
}

parser<protocol_model::base>::parser(parser &&other) noexcept :
	const_headers(other.m_headers),
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
	other.m_headers = &other.m_impl->m_headers;
}

parser<protocol_model::base> &parser<protocol_model::base>::operator=(parser &&other) noexcept
{
	if( this == &other )
		return *this;

	delete m_impl;
	m_impl = other.m_impl;
	m_headers = other.m_headers;

	other.m_impl = new impl(0xFFFF);
	other.m_headers = &other.m_impl->m_headers;
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::on_parse_begin(parse_begin_handler func)
{
	m_impl->m_parse_begin = std::move(func);
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::on_parse_cookie(parse_cookie_handler func)
{
	m_impl->m_parse_cookie = std::move(func);
	return *this;
}

error_code parser<protocol_model::base>::make_error_code(parse_errno errc)
{
	return http_nt::make_error_code(errc);
}

sys_expected<bool> parser<protocol_model::base>::append(const const_buffer &buf)
{
	using state_t = impl::state;
	std::string str_buf (
		static_cast<const char*>(buf.data()),
		buf.size()
	);
	if( str_buf.empty() )
		return { make_error_code(parse_errno::IDE) };

	else if( m_impl->m_state == state_t::finished )
		return { make_error_code(parse_errno::RE) };

	m_impl->m_src_buf += str_buf;
	if( m_impl->m_state <= state_t::reading_headers )
		return m_impl->parse_header();

	else if( m_impl->m_state == state_t::reading_length )
	{
		m_impl->parse_length();
		return true;
	}
	else if( m_impl->m_state == state_t::reading_eof )
	{
		m_impl->m_partial_body += m_impl->m_src_buf;
		m_impl->m_src_buf.clear();
		return false;
	}
	return m_impl->parse_chunked();
}

parser<protocol_model::base> &parser<protocol_model::base>::operator<<(const const_buffer &buf)
{
	append(buf);
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::reset()
{
	m_impl->reset();
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::skip_body(bool value) noexcept
{
	m_impl->m_skip_body = value;
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::read_until_eof(bool value) noexcept
{
	m_impl->m_read_until_eof = value;
	return *this;
}

sys_expected<bool> parser<protocol_model::base>::next_message()
{
	m_impl->reset(true);
	if( m_impl->m_src_buf.empty() )
		return false;
	return m_impl->parse_header();
}

bool parser<protocol_model::base>::finish_eof() noexcept
{
	if( m_impl->m_state != impl::state::reading_eof )
		return false;
	m_impl->m_state = impl::state::finished;
	return true;
}

std::string parser<protocol_model::base>::take_partial_body(size_t size)
{
	if( size == 0 )
		return {};
	else if( size > m_impl->m_partial_body.size() )
		size = m_impl->m_partial_body.size();

	auto res = m_impl->m_partial_body.substr(0,size);
	m_impl->m_partial_body.erase(0,size);
	return res;
}

std::string parser<protocol_model::base>::take_body()
{
	return std::exchange(m_impl->m_partial_body, {});
}

std::string parser<protocol_model::base>::take_pending_data()
{
	return std::exchange(m_impl->m_src_buf, {});
}

version_enum parser<protocol_model::base>::version() const noexcept
{
	return m_impl->m_version;
}

parser<protocol_model::base>::stage_t parser<protocol_model::base>::stage() const noexcept
{
	if( m_impl->m_state <= impl::state::reading_headers )
		return stage_t::header;
	else if( m_impl->m_state > impl::state::reading_headers and m_impl->m_state < impl::state::finished )
		return stage_t::body;
	else if( m_impl->m_partial_body.empty() )
		return stage_t::finished;
	return stage_t::body;
}

const parser<protocol_model::base>::chunk_attributes_t&
parser<protocol_model::base>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

parser<protocol_model::base> &parser<protocol_model::base>::unbind_parse_begin()
{
	m_impl->m_parse_begin = {};
	return *this;
}

parser<protocol_model::base> &parser<protocol_model::base>::unbind_parse_cookie()
{
	m_impl->m_parse_cookie = {};
	return *this;
}

} //namespace libgs::http_nt
