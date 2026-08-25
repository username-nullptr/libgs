
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

#include "url.h"
#include <libgs/core/algorithm/misc.h>
#include <libgs/core/string_vector.h>

namespace libgs::http_nt
{

class LIBGS_DECL_HIDDEN url::impl
{
	LIBGS_DISABLE_MOVE(impl)

public:
	explicit impl(std::string_view url = "/") {
		set(url);
	}
	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	void set(std::string_view url)
	{
		if( url.empty() )
			return ;

		auto addpth = parse_parameters(
			set_header(strtls::trimmed(url))
		);
		auto pos = addpth.find('/');
		if( pos == std::string::npos )
		{
			m_address = std::move(addpth);
			m_path = "/";
		}
		else
		{
			m_address = addpth.substr(0,pos);
			m_path = strtls::replace (
				from_percent_encoding(addpth.substr(pos)),
				"//", '/', false
			);
		}
		if( m_address.empty() )
		{
			m_address = "127.0.0.1";
			return ;
		}
		pos = m_address.rfind(':');
		if( pos == std::string::npos )
			m_port = m_protocol == "https" ? 443 : 80;
		else
		{
			m_port = *strtls::to_uint16(m_address.substr(pos+1)).or_else();
			m_address = m_address.substr(0,pos);
		}
	}

private:
	[[nodiscard]] std::string set_header(std::string resource_line)
	{
		if( resource_line.size() < 8 or strtls::to_lower(resource_line.substr(0,4)) != "http" )
			return "";

		if( resource_line[4] == 's' and resource_line.size() > 8 )
		{
			if( resource_line[5] == ':' and resource_line[6] == '/' and resource_line[7] == '/' )
			{
				m_protocol = "https";
				resource_line = resource_line.substr(8);
			}
		}
		else if( resource_line[4] == ':' )
		{
			if( resource_line[5] == '/' and resource_line[6] == '/' )
			{
				m_protocol = "http";
				resource_line = resource_line.substr(7);
			}
		}
		return resource_line;
	}

	[[nodiscard]] std::string parse_parameters(std::string resource_line)
	{
		auto pos = resource_line.find("?");
		if( pos == std::string::npos )
			return std::move(resource_line);

		auto addpth = resource_line.substr(0,pos);
		auto parameters_string = resource_line.substr(pos + 1);

		for(auto &para_str : string_vector::from_string(parameters_string, "&"))
		{
			pos = para_str.find("=");
			if( pos == std::string::npos )
			{
				para_str = from_percent_encoding(para_str);
				auto key = para_str;
				m_parameters.emplace_back(std::move(key), std::move(para_str));
			}
			else
			{
				m_parameters.emplace_back (
					from_percent_encoding(para_str.substr(0, pos)),
					from_percent_encoding(para_str.substr(pos + 1))
				);
			}
		}
		return addpth;
	}

public:
	std::string m_protocol = "http";
	std::string m_path = "/";
	std::string m_address = "127.0.0.1";
	uint16_t m_port = 80;
	parameters_t m_parameters {};
};

url::url(std::string_view url) :
	mutable_parameters(nullptr),
	m_impl(new impl(url))
{
	m_parameters = &m_impl->m_parameters;
}

url::url(std::string u) :
	url(std::string_view(u))
{

}

url::url(const char *u) :
	url(std::string_view(u))
{

}

url::url() : url(std::string_view())
{

}

url::~url()
{
	delete m_impl;
}

url::url(const url &other) :
	mutable_parameters(nullptr),
	m_impl(new impl(*other.m_impl))
{
	m_parameters = &m_impl->m_parameters;
}

url &url::operator=(const url &other)
{
	if( this != &other )
		*m_impl = *other.m_impl;
	return *this;
}

url::url(url &&other) noexcept :
	mutable_parameters(other.m_parameters),
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
	other.m_parameters = &other.m_impl->m_parameters;
}

url &url::operator=(url &&other) noexcept
{
	if( this == &other )
		return *this;

	delete m_impl;
	m_impl = other.m_impl;
	m_parameters = other.m_parameters;

	other.m_impl = new impl();
	other.m_parameters = &other.m_impl->m_parameters;
	return *this;
}

url &url::emplace(std::string_view url)
{
	m_impl->set(url);
	return *this;
}

url &url::set_address(std::string addr)
{
	m_impl->m_address = std::move(addr);
	return *this;
}

url &url::set_port(uint16_t port)
{
	m_impl->m_port = port;
	return *this;
}

url &url::set_path(std::string_view path)
{
	m_impl->set(path);
	return *this;
}

std::string_view url::protocol() const noexcept
{
	return m_impl->m_protocol;
}

std::string_view url::address() const noexcept
{
	return m_impl->m_address;
}

uint16_t url::port() const noexcept
{
	return m_impl->m_port;
}

std::string_view url::path() const noexcept
{
	return m_impl->m_path;
}

std::string url::to_string() const noexcept
{
	auto buf = std::format("{}://{}:{}",
		m_impl->m_protocol, m_impl->m_address, m_impl->m_port
	);
	if( m_impl->m_parameters.empty() )
		return buf;

	buf += '?';
	for(auto &[key,value] : m_impl->m_parameters)
		buf += key + "=" + value.to_string() + "&";
	buf.pop_back();
	return buf;
}

url::operator std::string() const noexcept
{
	return to_string();
}

} //namespace libgs::http_nt
