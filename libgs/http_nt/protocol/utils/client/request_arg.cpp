
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

#include "request_arg.h"
#include <libgs/http_nt/protocol/utils/core/compression.h>

namespace libgs::http_nt { namespace
{

[[nodiscard]] std::string base64_encode(std::string_view input)
{
	static constexpr std::string_view alphabet =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string output {};
	output.reserve((input.size() + 2) / 3 * 4);

	for(size_t offset=0; offset<input.size(); offset += 3)
	{
		uint32_t value = static_cast<unsigned char>(input[offset]) << 16;
		if( offset + 1 < input.size() )
			value |= static_cast<unsigned char>(input[offset + 1]) << 8;

		if( offset + 2 < input.size() )
			value |= static_cast<unsigned char>(input[offset + 2]);

		output += alphabet[(value >> 18) & 0x3F];
		output += alphabet[(value >> 12) & 0x3F];

		output += offset + 1 < input.size() ?
			alphabet[(value >> 6) & 0x3F] : '=';

		output += offset + 2 < input.size() ?
			alphabet[value & 0x3F] : '=';
	}
	return output;
}

[[nodiscard]] std::string basic_credentials
(std::string_view username, std::string_view password)
{
	if( username.find(':') != std::string_view::npos )
		invalid_argument::loc_throw("HTTP Basic user name cannot contain ':'.");
	return "Basic " + base64_encode(std::string(username) + ":" + std::string(password));
}

} //namespace

class LIBGS_DECL_HIDDEN request_arg::impl
{
public:
	impl()
	{
		if constexpr( gzip_available_v )
			m_headers[header_t::accept_encoding] = "gzip";
	}

public:
	headers_t m_headers {
		{ header_t::accept      , "*/*"                       },
		{ header_t::content_type, "text/plain; charset=utf-8" }
	};
	cookies_t m_cookies {};
	values_t m_chunk_attributes {};
};

request_arg::request_arg() :
	mutable_headers(nullptr),
	mutable_cookies(nullptr),
	mutable_chunk_attributes(nullptr),
	m_impl(new impl())
{
	m_headers = &m_impl->m_headers;
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_chunk_attributes;
}

request_arg::~request_arg()
{
	delete m_impl;
}

request_arg::request_arg(const request_arg &other) noexcept :
	mutable_headers(nullptr),
	mutable_cookies(nullptr),
	mutable_chunk_attributes(nullptr),
	m_impl(new impl(*other.m_impl))
{
	m_headers = &m_impl->m_headers;
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_chunk_attributes;
}

request_arg &request_arg::operator=(const request_arg &other) noexcept
{
	if( this != &other )
		*m_impl = *other.m_impl;
	return *this;
}

request_arg::request_arg(request_arg &&other) noexcept :
	mutable_headers(nullptr),
	mutable_cookies(nullptr),
	mutable_chunk_attributes(nullptr),
	m_impl(new impl(std::move(*other.m_impl)))
{
	m_headers = &m_impl->m_headers;
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_chunk_attributes;
}

request_arg &request_arg::operator=(request_arg &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

request_arg &request_arg::set_basic_auth(std::string_view username, std::string_view password)
{
	return set_header(header::authorization, basic_credentials(username, password));
}

request_arg &request_arg::set_bearer_auth(std::string_view token)
{
	return set_header(header::authorization, "Bearer " + std::string(token));
}

request_arg &request_arg::set_proxy_basic_auth(std::string_view username, std::string_view password)
{
	return set_header(header::proxy_authorization, basic_credentials(username, password));
}

} //namespace libgs::http_nt
