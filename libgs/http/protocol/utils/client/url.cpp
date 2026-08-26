
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

namespace libgs::http
{

class LIBGS_DECL_HIDDEN url::impl
{
	LIBGS_DISABLE_MOVE(impl)

public:
	explicit impl(std::string_view url = {}) {
		set(url);
	}
	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	void set(std::string_view url)
	{
		reset();
		if( url.empty() )
			return ;

		auto resource = set_header(strtls::trimmed(url));
		auto fragment = resource.find('#');

		if( fragment != std::string::npos )
			resource.erase(fragment);

		auto authority_end = resource.find_first_of("/?");
		auto authority = resource.substr(0, authority_end);

		auto path_query = authority_end == std::string::npos ?
			std::string("/") : resource.substr(authority_end);

		if( path_query.starts_with('?') )
			path_query.insert(path_query.begin(), '/');

		auto path = parse_parameters(std::move(path_query));
		set_path(path);

		if( authority.empty() or authority.find('@') != std::string::npos )
			invalid_argument::loc_throw("Invalid HTTP URL authority.");

		m_port = m_protocol == "https" ? 443 : 80;
		if( authority.starts_with('[') )
		{
			auto close = authority.find(']');
			if( close == std::string::npos )
				invalid_argument::loc_throw("Invalid IPv6 URL authority.");

			m_address = authority.substr(1, close - 1);
			if( close + 1 < authority.size() )
			{
				if( authority[close + 1] != ':' )
					invalid_argument::loc_throw("Invalid HTTP URL authority.");
				set_port_text(authority.substr(close + 2));
			}
		}
		else
		{
			auto colon = authority.rfind(':');
			if( colon != std::string::npos and authority.find(':') == colon )
			{
				m_address = authority.substr(0, colon);
				set_port_text(authority.substr(colon + 1));
			}
			else
				m_address = std::move(authority);
		}
		if( m_address.empty() )
			invalid_argument::loc_throw("HTTP URL host is empty.");
	}

	void set_path(std::string_view path)
	{
		auto value = from_percent_encoding(strtls::trimmed(path));
		if( value.empty() )
			value = "/";

		else if( not value.starts_with('/') )
			value.insert(value.begin(), '/');

		m_path = strtls::replace (
			std::move(value), "//", '/', false
		);
	}

private:
	void set_port_text(std::string_view text)
	{
		auto port = strtls::to_uint16(text);
		if( not port or *port == 0 )
			invalid_argument::loc_throw("Invalid HTTP URL port.");
		m_port = *port;
	}

	void reset()
	{
		m_protocol = "http";
		m_path = "/";
		m_address = "127.0.0.1";
		m_port = 80;
		m_parameters.clear();
	}

	[[nodiscard]] std::string set_header(const std::string &resource_line)
	{
		auto lower = strtls::to_lower(resource_line);
		if( lower.starts_with("https://") )
		{
			m_protocol = "https";
			return resource_line.substr(8);
		}
		if( lower.starts_with("http://") )
		{
			m_protocol = "http";
			return resource_line.substr(7);
		}
		invalid_argument::loc_throw("HTTP URL must use the http or https scheme.");
	}

	[[nodiscard]] std::string parse_parameters(std::string resource_line)
	{
		auto pos = resource_line.find('?');
		if( pos == std::string::npos )
			return resource_line;

		auto addpth = resource_line.substr(0,pos);

		for(auto parameters_string = resource_line.substr(pos + 1);
			auto &para_str : string_vector::from_string(parameters_string, "&"))
		{
			pos = para_str.find('=');
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

url::url(const std::string &u) :
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
	m_impl->set_path(path);
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
	auto authority = m_impl->m_address;
	if( authority.find(':') != std::string::npos and not authority.starts_with('[') )
		authority = '[' + authority + ']';

	auto buf = std::format("{}://{}:{}{}",
		m_impl->m_protocol, authority, m_impl->m_port,
		to_percent_encoding(m_impl->m_path, '/')
	);
	if( m_impl->m_parameters.empty() )
		return buf;
	buf += '?';

	for(auto &[key,value] : m_impl->m_parameters)
	{
		buf += to_percent_encoding(key) + "=" +
			to_percent_encoding(value.to_string()) + "&";
	}
	buf.pop_back();
	return buf;
}

url::operator std::string() const noexcept
{
	return to_string();
}

url url::resolve(const url &base, std::string_view reference)
{
	auto value = strtls::trimmed(reference);
	if( auto fragment = value.find('#'); fragment != std::string::npos )
		value.erase(fragment);

	auto lower = strtls::to_lower(value);
	if( lower.starts_with("http://") or lower.starts_with("https://") )
		return { value };

	auto host = std::string(base.address());
	if( host.find(':') != std::string::npos and not host.starts_with('[') )
		host = '[' + host + ']';

	auto origin = std::format("{}://{}:{}", base.protocol(), host, base.port());
	if( value.starts_with("//") )
		return { std::string(base.protocol()) + ":" + value };

	if( value.empty() )
		return base;

	if( value.front() == '/' )
		return { origin + value };

	if( value.front() == '?' )
		return { origin + std::string(base.path()) + value };

	auto path = std::string(base.path());
	path.erase(path.rfind('/') + 1);
	path += value;

	std::vector<std::string> segments {};
	for(auto &segment : string_vector::from_string(path, '/'))
	{
		if( segment.empty() or segment == "." )
			continue;
		if( segment == ".." )
		{
			if( not segments.empty() )
				segments.pop_back();
		}
		else
			segments.emplace_back(std::move(segment));
	}
	path = "/";
	for(auto &segment : segments)
		path += segment + '/';

	if( not value.ends_with('/') and path.size() > 1 )
		path.pop_back();
	return { origin + path };
}

} //namespace libgs::http
