
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

namespace libgs::http { namespace
{

[[nodiscard]] bool ascii_alpha(char value) noexcept
{
	return (value >= 'a' and value <= 'z') or
		(value >= 'A' and value <= 'Z');
}

[[nodiscard]] bool valid_scheme(std::string_view value) noexcept
{
	if( value.empty() or not ascii_alpha(value.front()) )
		return false;
	for(char item : value.substr(1))
	{
		if( not ascii_alpha(item) and not (item >= '0' and item <= '9') and
			item != '+' and item != '-' and item != '.' )
		{
			return false;
		}
	}
	return true;
}

} //namespace

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

		m_valid = false;
		try {
			parse(url);
			refresh_validity();
		}
		catch(const std::invalid_argument&) {
			invalidate();
		}
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
		refresh_validity();
	}

	void refresh_validity() noexcept
	{
		const bool http_scheme =
			m_protocol == "http" or m_protocol == "https";
		m_valid = not m_protocol.empty() and not m_host.empty() and
			(not http_scheme or m_port != 0) and not m_path.empty() and
			m_path.front() == '/';
	}

private:
	void parse(std::string_view url)
	{
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
			invalid_argument::loc_throw("Invalid URL authority.");

		m_port = default_port(m_protocol);
		if( authority.starts_with('[') )
		{
			auto close = authority.find(']');
			if( close == std::string::npos )
				invalid_argument::loc_throw("Invalid IPv6 URL authority.");

			m_host = authority.substr(1, close - 1);
			if( close + 1 < authority.size() )
			{
				if( authority[close + 1] != ':' )
						invalid_argument::loc_throw("Invalid URL authority.");
				set_port_text(authority.substr(close + 2));
			}
		}
		else
		{
			auto colon = authority.rfind(':');
			if( colon != std::string::npos and authority.find(':') == colon )
			{
				m_host = authority.substr(0, colon);
				set_port_text(authority.substr(colon + 1));
			}
			else
				m_host = std::move(authority);
		}
		if( m_host.empty() )
			invalid_argument::loc_throw("URL host is empty.");
	}

	[[nodiscard]] static uint16_t default_port(std::string_view scheme) noexcept
	{
		if( scheme == "http" )
			return 80;
		if( scheme == "https" )
			return 443;
		return 0;
	}

	void set_port_text(std::string_view text)
	{
		auto port = strtls::to_uint16(text);
		if( not port or *port == 0 )
			invalid_argument::loc_throw("Invalid URL port.");
		m_port = *port;
	}

	void reset()
	{
		m_protocol = "http";
		m_path = "/";
		m_host = "127.0.0.1";
		m_port = 80;
		m_parameters.clear();
		m_valid = true;
	}

	void invalidate() noexcept
	{
		m_protocol.clear();
		m_path.clear();
		m_host.clear();
		m_port = 0;
		m_parameters.clear();
		m_valid = false;
	}

	[[nodiscard]] std::string set_header(const std::string &resource_line)
	{
		auto scheme_end = resource_line.find("://");
		if( scheme_end == std::string::npos or scheme_end == 0 )
			invalid_argument::loc_throw("URL scheme is missing.");

		auto scheme = std::string_view(resource_line).substr(0, scheme_end);
		if( not valid_scheme(scheme) )
			invalid_argument::loc_throw("Invalid URL scheme.");

		m_protocol = strtls::to_lower(scheme);
		return resource_line.substr(scheme_end + 3);
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
	std::string m_host = "127.0.0.1";
	uint16_t m_port = 80;
	parameters_t m_parameters {};
	bool m_valid = true;
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
	m_impl->m_host = std::move(addr);
	m_impl->refresh_validity();
	return *this;
}

url &url::set_port(uint16_t port)
{
	m_impl->m_port = port;
	m_impl->refresh_validity();
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

std::string_view url::host() const noexcept
{
	return m_impl->m_host;
}

uint16_t url::port() const noexcept
{
	return m_impl->m_port;
}

std::string_view url::path() const noexcept
{
	return m_impl->m_path;
}

bool url::is_valid() const noexcept
{
	return m_impl->m_valid;
}

std::string url::to_string() const noexcept
{
	if( not is_valid() )
		return {};

	auto authority = m_impl->m_host;
	if( authority.find(':') != std::string::npos and not authority.starts_with('[') )
		authority = '[' + authority + ']';

	auto buf = std::format("{}://{}", m_impl->m_protocol, authority);
	if( m_impl->m_port != 0 )
		buf += ':' + std::to_string(m_impl->m_port);
	buf += to_percent_encoding(m_impl->m_path, '/');
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

	if( auto scheme_end = value.find("://"); scheme_end != std::string::npos )
	{
		if( valid_scheme(std::string_view(value).substr(0, scheme_end)) )
			return { value };
	}

	auto host = std::string(base.host());
	if( host.find(':') != std::string::npos and not host.starts_with('[') )
		host = '[' + host + ']';

	auto origin = std::format("{}://{}", base.protocol(), host);
	if( base.port() != 0 )
		origin += ':' + std::to_string(base.port());
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
