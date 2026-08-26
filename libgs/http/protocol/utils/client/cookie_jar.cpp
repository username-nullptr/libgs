/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#include "cookie_jar.h"
#include <libgs/http/protocol/utils/core/conditional.h>
#include <libgs/core/spin_mutex.h>

namespace libgs::http { namespace
{

[[nodiscard]] std::string canonical_host(std::string_view host)
{
	auto result = strtls::to_lower(strtls::trimmed(host));
	while( not result.empty() and result.back() == '.' )
		result.pop_back();
	return result;
}

[[nodiscard]] bool domain_match(std::string_view host, std::string_view domain) noexcept
{
	return host == domain or (host.size() > domain.size() and
		host.ends_with(domain) and host[host.size() - domain.size() - 1] == '.');
}

[[nodiscard]] std::string default_path(std::string_view request_path)
{
	if( request_path.empty() or request_path.front() != '/' )
		return "/";
	auto slash = request_path.rfind('/');
	if( slash == 0 )
		return "/";
	return std::string(request_path.substr(0, slash));
}

[[nodiscard]] bool path_match(std::string_view request_path,
	std::string_view cookie_path) noexcept
{
	if( request_path == cookie_path )
		return true;
	if( not request_path.starts_with(cookie_path) )
		return false;
	return cookie_path.ends_with('/') or
		(request_path.size() > cookie_path.size() and
		 request_path[cookie_path.size()] == '/');
}

[[nodiscard]] optional<int64_t> signed_seconds(const value &input) noexcept
{
	auto text = strtls::trimmed(input.to_string());
	int64_t result = 0;

	if( auto [end,error] = std::from_chars(text.data(), text.data() + text.size(), result);
		error != std::errc() or end != text.data() + text.size() )
		return nullopt;
	return result;
}

[[nodiscard]] const value *attribute(const cookie &input, std::string_view name) noexcept
{
	for(auto &[key,item] : input.attributes())
	{
		if( strtls::to_lower(key) == strtls::to_lower(name) )
			return &item;
	}
	return nullptr;
}

[[nodiscard]] bool boolean_attribute(const cookie &input, std::string_view name) noexcept
{
	auto item = attribute(input, name);
	if( not item )
		return false;
	auto result = item->to_bool();
	return result and *result;
}

} //namespace

class LIBGS_DECL_HIDDEN cookie_jar::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;
	void remove_expired(std::chrono::system_clock::time_point now)
	{
		std::erase_if(m_entries, [&](const entry &item) {
			return item.expires and *item.expires <= now;
		});
	}

public:
	uint64_t m_creation_counter = 0;
	std::vector<entry> m_entries {};
	spin_mutex m_mutex {};
};

cookie_jar::cookie_jar() :
	m_impl(new impl())
{

}

cookie_jar::~cookie_jar()
{
	delete m_impl;
}

bool cookie_jar::store(const url &origin, std::string name, const cookie &input)
{
	if( name.empty() )
		return false;

	auto host = canonical_host(origin.address());
	if( host.empty() )
		return false;

	entry item {};
	item.name = std::move(name);
	item.value = input.value().to_string();
	item.domain = host;
	item.path = default_path(origin.path());
	item.secure = boolean_attribute(input, cookie_attribute::secure);
	item.http_only = boolean_attribute(input, cookie_attribute::http_only);

	if( item.secure and strtls::to_lower(origin.protocol()) != "https" )
		return false;

	if( auto domain = attribute(input, cookie_attribute::domain) )
	{
		auto candidate = canonical_host(domain->to_string());
		while( candidate.starts_with('.') )
			candidate.erase(candidate.begin());

		if( candidate.empty() or not domain_match(host, candidate) )
			return false;

		item.domain = std::move(candidate);
		item.host_only = false;
	}
	if( auto path = attribute(input, cookie_attribute::path) )
	{
		auto candidate = path->to_string();
		if( not candidate.empty() and candidate.front() == '/' )
			item.path = std::move(candidate);
	}
	auto now = std::chrono::system_clock::now();
	bool remove = false;
	bool valid_max_age = false;

	if( auto max_age = attribute(input, cookie_attribute::max_age) )
	{
		if( auto seconds = signed_seconds(*max_age) )
		{
			valid_max_age = true;
			remove = *seconds <= 0;

			if( not remove )
				item.expires = now + std::chrono::seconds(*seconds);
		}
	}
	if( not valid_max_age )
	{
		if( auto expires = attribute(input, cookie_attribute::expires) )
		{
			item.expires = parse_http_date(expires->to_string());
			if( not item.expires )
			{
				if( auto seconds = signed_seconds(*expires) )
					item.expires = std::chrono::system_clock::time_point(std::chrono::seconds(*seconds));
			}
			remove = item.expires and *item.expires <= now;
		}
	}

	std::scoped_lock lock(m_impl->m_mutex);
	m_impl->remove_expired(now);

	auto same_cookie = [&](const entry &old) {
		return old.name == item.name and old.domain == item.domain and old.path == item.path;
	};
	auto existing = std::ranges::find_if (
		m_impl->m_entries, same_cookie
	);
	if( remove )
	{
		if( existing != m_impl->m_entries.end() )
			m_impl->m_entries.erase(existing);
		return true;
	}
	if( existing != m_impl->m_entries.end() )
	{
		item.creation_index = existing->creation_index;
		*existing = std::move(item);
	}
	else
	{
		item.creation_index = m_impl->m_creation_counter++;
		m_impl->m_entries.emplace_back(std::move(item));
	}
	return true;
}

void cookie_jar::store(const url &origin,
	const std::vector<std::pair<std::string,cookie>> &values)
{
	for(auto &[name,item] : values)
		store(origin, name, item);
}

cookie_values cookie_jar::cookies_for(const url &target)
{
	auto host = canonical_host(target.address());
	auto path = target.path().empty() ? std::string_view("/") : target.path();

	auto secure = strtls::to_lower(target.protocol()) == "https";
	auto now = std::chrono::system_clock::now();

	std::scoped_lock lock(m_impl->m_mutex);
	m_impl->remove_expired(now);

	auto matches = m_impl->m_entries;
	std::erase_if(matches, [&](const entry &item)
	{
		return (item.host_only ? host != item.domain : not domain_match(host, item.domain)) or
			not path_match(path, item.path) or (item.secure and not secure);
	});
	std::ranges::sort(matches, [](const entry &lhs, const entry &rhs)
	{
		if( lhs.path.size() != rhs.path.size() )
			return lhs.path.size() > rhs.path.size();
		return lhs.creation_index < rhs.creation_index;
	});

	cookie_values result {};
	for(auto &item : matches)
	{
		if( not result.contains(item.name) )
			result[item.name] = item.value;
	}
	return result;
}

std::vector<cookie_jar::entry> cookie_jar::entries()
{
	std::scoped_lock lock(m_impl->m_mutex);
	m_impl->remove_expired(std::chrono::system_clock::now());
	return m_impl->m_entries;
}

size_t cookie_jar::size()
{
	std::scoped_lock lock(m_impl->m_mutex);
	m_impl->remove_expired(std::chrono::system_clock::now());
	return m_impl->m_entries.size();
}

void cookie_jar::clear() noexcept
{
	std::scoped_lock lock(m_impl->m_mutex);
	m_impl->m_entries.clear();
}

} //namespace libgs::http
