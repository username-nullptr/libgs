
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

#include "generator.h"
#include <libgs/http_nt/protocol/utils/core/generator.h>

namespace libgs::http_nt
{

class LIBGS_DECL_HIDDEN generator<model::server>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using generator_ptr = std::shared_ptr<base_generator>;

public:
	impl(version_enum version, const headers_t &req_headers) :
		m_req_headers(&req_headers)
	{
		version_t::check(version);
		if( version == version::v10 )
			m_generator = std::make_shared<base_generator_v10>();
		else if( version == version::v11 )
			m_generator = std::make_shared<base_generator_v11>();
		// else ... ...
	}

	[[nodiscard]] bool request_chunked() const
	{
		if( m_generator->version() < version::v11 )
			return false;

		auto it = m_req_headers->find(header::transfer_encoding);
		return it != m_req_headers->end() and
			strtls::to_lower(*it->second) == "chunked";
	}

public:
	const headers_t *m_req_headers = nullptr;
	generator_ptr m_generator {};

	status_enum m_status = status::ok;
	cookies_t m_cookies {};
};

generator<model::server>::generator(version_enum version, const headers_t &req_headers) :
	mutable_headers(nullptr),
	mutable_cookies(nullptr),
	mutable_chunk_attributes(nullptr),
	m_impl(new impl(version, req_headers))
{
	m_headers = &m_impl->m_generator->headers();
	m_cookies = &m_impl->m_cookies;
	m_chunk_attributes = &m_impl->m_generator->chunk_attributes();
}

generator<model::server>::generator(const headers_t &req_headers) :
	generator(version::v11, req_headers)
{

}

generator<model::server>::~generator()
{
	delete m_impl;
}

generator<model::server>::generator(generator &&other) noexcept :
	mutable_headers(other.m_headers),
	mutable_cookies(other.m_cookies),
	mutable_chunk_attributes(other.m_chunk_attributes),
	m_impl(other.m_impl)
{
	other.m_impl = new impl(version(), *m_impl->m_req_headers);
	other.m_headers = &other.m_impl->m_generator->headers();
	other.m_cookies = &other.m_impl->m_cookies;
	other.m_chunk_attributes = &other.m_impl->m_generator->chunk_attributes();
}

generator<model::server> &generator<model::server>::operator=(generator &&other) noexcept
{
	if( this == &other )
		return *this;

	delete m_impl;
	m_impl = other.m_impl;
	m_headers = other.m_headers;
	m_cookies = other.m_cookies;
	m_chunk_attributes = other.m_chunk_attributes;

	other.m_impl = new impl(version(), *m_impl->m_req_headers);
	other.m_headers = &other.m_impl->m_generator->headers();
	other.m_cookies = &other.m_impl->m_cookies;
	other.m_chunk_attributes = &other.m_impl->m_generator->chunk_attributes();
	return *this;
}

generator<model::server> &generator<model::server>::set_status(status_enum status)
{
	status::check(status);
	m_impl->m_status = status;
	return *this;
}

status_enum generator<model::server>::status() const noexcept
{
	return m_impl->m_status;
}

std::string generator<model::server>::header_data(size_t body_size)
{
	if( m_impl->m_generator->state() != generator_state::header )
		return {};

	std::string buf;
	buf.reserve(4096);

	buf = std::format("HTTP/{} {} {}\r\n",
		version::string(version()), m_impl->m_status,
		status::description(m_impl->m_status)
	);
	m_impl->m_generator->unset_header("set-cookie");

	if( m_impl->request_chunked() )
		m_impl->m_generator->set_header(header::transfer_encoding, "chunked");
	buf += m_impl->m_generator->header_data(body_size);

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

std::string generator<model::server>::body_data(const const_buffer &buffer)
{
	return m_impl->m_generator->body_data(buffer);
}

std::string generator<model::server>::chunk_end_data(const headers_t &headers)
{
	return m_impl->m_generator->chunk_end_data(headers);
}

version_enum generator<model::server>::version() const noexcept
{
	return m_impl->m_generator->version();
}

generator_state generator<model::server>::pro_state() const noexcept
{
	return m_impl->m_generator->state();
}

generator<model::server> &generator<model::server>::reset() noexcept
{
	m_impl->m_status = status::ok;
	m_impl->m_cookies.clear();
	m_impl->m_generator.reset();
	return *this;
}

} //namespace libgs::http_nt