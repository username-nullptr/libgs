
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

#include "response_helper.h"

namespace libgs::http
{

/*
#define libgs_http_detail_static_string(_type, ...) \
	static constexpr const _type *colon                 = __va_args__##": "                             ; \
	static constexpr const _type *line_break            = __va_args__##"\r\n"                           ; \
	static constexpr const _type *bytes                 = __va_args__##"bytes"                          ; \
	static constexpr const _type *bytes_start           = __va_args__##"bytes="                         ; \
	static constexpr const _type *range_format          = __va_args__##"{}-{},"                         ; \
	static constexpr const _type *content_range_format  = __va_args__##"{}-{}/{}"                       ; \
	static constexpr const _type *content_type_boundary = __va_args__##"multipart/byteranges; boundary="; \
*/

class response_helper::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl(version_enum version, const headers_t &req_headers) :
		m_req_headers(&req_headers)
	{
		http::version::check(version);
		if( version == version::v10 )
			m_helper = std::make_shared<helper_base_v10>();
		else if( version == version::v11 )
			m_helper = std::make_shared<helper_base_v11>();
		// else ... ...
	}

	[[nodiscard]] bool request_chunked() const
	{
		if( m_helper->version() < version::v11 )
			return false;

		auto it = m_req_headers->find(header::transfer_encoding);
		return it != m_req_headers->end() and
			strtls::to_lower(*it->second) == "chunked";
	}

public:
	const headers_t *m_req_headers = nullptr;
	next_layer_t m_helper {};

	status_enum m_status = status::ok;
	cookies_t m_cookies {};
};

response_helper::response_helper(version_enum version, const headers_t &req_headers) :
	m_impl(new impl(version, req_headers))
{

}

response_helper::response_helper(const headers_t &req_headers) :
	m_impl(new impl(version::v11, req_headers))
{

}

response_helper::~response_helper()
{
	delete m_impl;
}

response_helper::response_helper(response_helper &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(version(), *m_impl->m_req_headers);
}

response_helper &response_helper::operator=(response_helper &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(version(), *m_impl->m_req_headers);
	return *this;
}

const headers &response_helper::headers() const noexcept
{
	return m_impl->m_helper->headers();
}

headers &response_helper::headers() noexcept
{
	return m_impl->m_helper->headers();
}

const cookies &response_helper::cookies() const noexcept
{
	return m_impl->m_cookies;
}

cookies &response_helper::cookies() noexcept
{
	return m_impl->m_cookies;
}

response_helper &response_helper::set_chunk_attribute(value_t attr) noexcept
{
	m_impl->m_helper->set_chunk_attribute(std::move(attr));
	return *this;
}

response_helper &response_helper::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_helper->unset_chunk_attribute(attr);
	return *this;
}

const std::set<value> &response_helper::chunk_attributes() const noexcept
{
	return m_impl->m_helper->chunk_attributes();
}

std::set<value> &response_helper::chunk_attributes() noexcept
{
	return m_impl->m_helper->chunk_attributes();
}

response_helper &response_helper::set_status(status_enum status)
{
	status::check(status);
	m_impl->m_status = status;
	return *this;
}

status_enum response_helper::status() const noexcept
{
	return m_impl->m_status;
}

std::string response_helper::header_data(size_t body_size)
{
	if( m_impl->m_helper->state() != helper_state::header )
		return {};

	std::string buf;
	buf.reserve(4096);

	buf = std::format("HTTP/{} {} {}\r\n",
		version::string(version()), m_impl->m_status,
		status::description(m_impl->m_status)
	);
	m_impl->m_helper->unset_header("set-cookie");

	if( m_impl->request_chunked() )
		m_impl->m_helper->set_header(header::transfer_encoding, "chunked");
	buf += m_impl->m_helper->header_data(body_size);

	for(auto &[ckey,cookie] : m_impl->m_cookies)
	{
		buf += "set-cookie: " + ckey + "=" + *cookie.value() + ";";
		for(auto &[akey,attr] : cookie.attributes())
			buf += akey + "=" + *attr + ";";

		buf.pop_back();
		buf += "\r\n";
	}
	return buf + "\r\n";
}

std::string response_helper::body_data(const const_buffer &buffer)
{
	return m_impl->m_helper->body_data(buffer);
}

std::string response_helper::chunk_end_data(const headers_t &headers)
{
	return m_impl->m_helper->chunk_end_data(headers);
}

version_enum response_helper::version() const noexcept
{
	return m_impl->m_helper->version();
}

helper_state response_helper::pro_state() const noexcept
{
	return m_impl->m_helper->state();
}

response_helper::next_layer_t response_helper::next_layer() noexcept
{
	return m_impl->m_helper;
}

response_helper &response_helper::reset() noexcept
{
	m_impl->m_status = status::ok;
	m_impl->m_cookies.clear();
	m_impl->m_helper.reset();
	return *this;
}

} //namespace libgs::http