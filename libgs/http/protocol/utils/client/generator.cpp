
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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
#include <libgs/core/algorithm/misc.h>

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN generator<model::client>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;
	explicit impl(version_enum version, request_arg_t request) :
		m_req_arg(std::move(request))
	{
		version_t::check(version);
		if( version == version::v10 )
			m_generator = std::make_shared<base_generator_v10>();
		else if( version == version::v11 )
			m_generator = std::make_shared<base_generator_v11>();
		// else ... ...
	}

	next_layer_t m_generator;
	request_arg_t m_req_arg;
	headers_t m_auto_headers;
};

generator<model::client>::generator(request_arg_t request) :
	generator(version_enum::v11, std::move(request))
{

}

generator<model::client>::generator(version_enum version, request_arg_t request) :
	m_impl(new impl(version, std::move(request)))
{

}

generator<model::client>::~generator()
{
	delete m_impl;
}

generator<model::client>::generator(generator &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

generator<model::client> &generator<model::client>::operator=(generator &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

generator<model::client> &generator<model::client>::set_arg(request_arg_t arg)
{
	m_impl->m_req_arg = std::move(arg);
	return *this;
}

const request_arg &generator<model::client>::arg() const noexcept
{
	return m_impl->m_req_arg;
}

request_arg &generator<model::client>::arg() noexcept
{
	return m_impl->m_req_arg;
}

std::string generator<model::client>::header_data(method_enum method, size_t body_size)
{
	if( m_impl->m_generator->state() != generator_state::header )
		return {};

	auto &arg = this->arg();
	auto &url = arg.url();

	auto buf = std::string(method::string(method)) + " ";
	{
		auto path = to_percent_encoding(url.path(), '/');
		if( not url.parameters().empty() )
		{
			path += "?";
			for(auto &[key,value] : url.parameters())
				path += to_percent_encoding(key) + "=" + to_percent_encoding(*value) + "&";
			path.pop_back();
		}
		buf += path + " HTTP/"
			+ version::string(m_impl->m_generator->version())
			+ "\r\n";
	}
	for(auto &[key,value] : arg.headers())
		m_impl->m_generator->set_header(key, value);

	buf += m_impl->m_generator->header_data(body_size);
	if( not arg.cookies().empty() )
	{
		buf += "Cookie: ";
		for(auto &[key,value] : arg.cookies())
			buf += key + "=" + *value + ";";
		buf += "\r\n";
	}
	return buf + "\r\n";
}

std::string generator<model::client>::body_data(const const_buffer &buffer)
{
	return m_impl->m_generator->body_data(buffer);
}

std::string generator<model::client>::chunk_end_data(const headers_t &headers)
{
	return m_impl->m_generator->chunk_end_data(headers);
}

version_enum generator<model::client>::version() const noexcept
{
	return m_impl->m_generator->version();
}

generator<model::client> &generator<model::client>::reset() noexcept
{
	m_impl->m_auto_headers.clear();
	m_impl->m_generator.reset();
	return *this;
}

} //namespace libgs::http::protocol