
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

#ifndef LIBGS_HTTP_DETAIL_COOKIE_H
#define LIBGS_HTTP_DETAIL_COOKIE_H

namespace libgs::http
{

inline cookie::cookie()
{
	m_attributes[cookie_attribute::path] = "/";
}

inline cookie::cookie(value_t value) :
	m_value(std::move(value))
{

}

cookie &cookie::set_value(value_t value) noexcept
{
	m_value = std::move(value);
	return *this;
}

inline cookie &cookie::operator=(value_t v) noexcept
{
	m_value = std::move(v);
	return *this;
}

template <core_concepts::value_get<char> T>
T cookie::value() noexcept
{
	return m_value.get<T>();
}

inline cookie::value_t cookie::value() noexcept
{
	return m_value;
}

inline cookie::operator value_t() noexcept
{
	return m_value;
}

inline std::string cookie::domain() const
{
	auto it = m_attributes.find(cookie_attribute::domain);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::domain: key 'Domain' not exists."
		);
	}
	return it->second.to_string();
}

inline std::string cookie::path() const
{
	auto it = m_attributes.find(cookie_attribute::path);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::path: key 'Path' not exists."
		);
	}
	return it->second.to_string();
}

inline std::string cookie::same_site() const
{
	auto it = m_attributes.find(cookie_attribute::same_site);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::same_site: key 'SameSite' not exists."
		);
	}
	return it->second.to_string();
}

inline std::string cookie::priority() const
{
	auto it = m_attributes.find(cookie_attribute::priority);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::priority: key 'Priority' not exists."
		);
	}
	return it->second.to_string();
}

inline uint64_t cookie::expires() const
{
	auto it = m_attributes.find(cookie_attribute::expires);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::expires: key 'Expires' not exists."
		);
	}
	return it->second.get<uint64_t>();
}

inline uint64_t cookie::max_age() const
{
	auto it = m_attributes.find(cookie_attribute::max_age);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::max_age: key 'Max-age' not exists."
		);
	}
	return it->second.get<uint64_t>();
}

inline size_t cookie::size() const
{
	auto it = m_attributes.find(cookie_attribute::size);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::cookies_size: key 'Size' not exists."
		);
	}
	return it->second.get<size_t>();
}

inline bool cookie::http_only() const
{
	auto it = m_attributes.find(cookie_attribute::http_only);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::http_only: key 'HttpOnly' not exists."
		);
	}
	return it->second.to_bool();
}

inline bool cookie::secure() const
{
	auto it = m_attributes.find(cookie_attribute::secure);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::secure: key 'Secure' not exists."
		);
	}
	return it->second.to_bool();
}

inline std::string cookie::domain_or(const value_t &def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::domain);
	return it == m_attributes.end() ? *def_value : *it->second;
}

inline std::string cookie::domain_or(value_t &&def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::domain);
	return it == m_attributes.end() ? *std::move(def_value) : *it->second;
}

inline std::string cookie::path_or(const value_t &def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::path);
	return it == m_attributes.end() ? *def_value : *it->second;
}

inline std::string cookie::path_or(value_t &&def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::path);
	return it == m_attributes.end() ? *std::move(def_value) : *it->second;
}

inline std::string cookie::same_site_or(const value_t &def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::same_site);
	return it == m_attributes.end() ? *def_value : *it->second;
}

inline std::string cookie::same_site_or(value_t &&def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::same_site);
	return it == m_attributes.end() ? *std::move(def_value) : *it->second;
}

inline std::string cookie::priority_or(const value_t &def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::priority);
	return it == m_attributes.end() ? *def_value : *it->second;
}

inline std::string cookie::priority_or(value_t &&def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::priority);
	return it == m_attributes.end() ? *std::move(def_value) : *it->second;
}

inline uint64_t cookie::expires_or(uint64_t def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::expires);
	return it == m_attributes.end() ? def_value : it->second.get<uint64_t>();
}

inline uint64_t cookie::max_age_or(uint64_t def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::max_age);
	return it == m_attributes.end() ? def_value : it->second.get<uint64_t>();
}

inline size_t cookie::size_or(size_t def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::size);
	return it == m_attributes.end() ? def_value : it->second.get<size_t>();
}

inline bool cookie::http_only_or(bool def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::http_only);
	return it == m_attributes.end() ? def_value : it->second.to_bool();
}

inline bool cookie::secure_or(bool def_value) const noexcept
{
	auto it = m_attributes.find(cookie_attribute::secure);
	return it == m_attributes.end() ? def_value : it->second.to_bool();
}

inline cookie &cookie::set_domain(value_t domain)
{
	m_attributes[cookie_attribute::domain] = std::move(domain);
	return *this;
}

inline cookie &cookie::set_path(value_t path)
{
	m_attributes[cookie_attribute::path] = std::move(path);
	return *this;
}

inline cookie &cookie::set_same_site(value_t sst)
{
	m_attributes[cookie_attribute::same_site] = std::move(sst);
	return *this;
}

inline cookie &cookie::set_priority(value_t pt)
{
	m_attributes[cookie_attribute::priority] = std::move(pt);
	return *this;
}

inline cookie &cookie::set_expires(uint64_t seconds)
{
	m_attributes[cookie_attribute::expires] = seconds;
	return *this;
}

inline cookie &cookie::set_max_age(uint64_t seconds)
{
	m_attributes[cookie_attribute::max_age] = seconds;
	return *this;
}

inline cookie &cookie::set_size(size_t size)
{
	m_attributes[cookie_attribute::size] = size;
	return *this;
}

inline cookie &cookie::set_http_only(bool flag)
{
	m_attributes[cookie_attribute::http_only] = flag;
	return *this;
}

inline cookie &cookie::set_secure(bool flag)
{
	m_attributes[cookie_attribute::secure] = flag;
	return *this;
}

inline cookie &cookie::unset_domain()
{
	m_attributes.erase(cookie_attribute::domain);
	return *this;
}

inline cookie &cookie::unset_path()
{
	m_attributes.erase(cookie_attribute::path);
	return *this;
}

inline cookie &cookie::unset_same_site()
{
	m_attributes.erase(cookie_attribute::same_site);
	return *this;
}

inline cookie &cookie::unset_priority()
{
	m_attributes.erase(cookie_attribute::priority);
	return *this;
}

inline cookie &cookie::unset_expires()
{
	m_attributes.erase(cookie_attribute::expires);
	return *this;
}

inline cookie &cookie::unset_max_age()
{
	m_attributes.erase(cookie_attribute::max_age);
	return *this;
}

inline cookie &cookie::unset_size()
{
	m_attributes.erase(cookie_attribute::size);
	return *this;
}

inline cookie &cookie::unset_http_only()
{
	m_attributes.erase(cookie_attribute::http_only);
	return *this;
}

inline cookie &cookie::unset_secure()
{
	m_attributes.erase(cookie_attribute::secure);
	return *this;
}

template <core_concepts::value_get<char> T>
T cookie::attribute(const core_concepts::text_p<char> auto &key) const
{
	auto it = m_attributes.find(key);
	if( it == m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::cookie::attributes: key '{}' not exists.", key
		);
	}
	return it->second.template get<T>();
}

template <core_concepts::text_arg_p<char> T>
decltype(auto) cookie::attribute_or
(const core_concepts::text_p<char> auto &key, T &&def_value) const noexcept
{
	auto it = m_attributes.find(key);
	using def_t = std::remove_cvref_t<T>;

	if constexpr( is_string_v<def_t, char> )
	{
		return it == m_attributes.end() ?
			strtls::to_string(std::forward<T>(def_value)) : *it->second;
	}
	else
	{
		return it == m_attributes.end() ? std::forward<T>(def_value) :
			it->second.template get<def_t>();
	}
}

cookie &cookie::set_attribute(core_concepts::text_p<char> auto &&key, value_t attr) noexcept
{
	m_attributes[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(attr);
	return *this;
}

cookie &cookie::unset_attribute(const core_concepts::text_p<char> auto &key) noexcept
{
	m_attributes.erase(strtls::to_string(key));
	return *this;
}

inline const cookie::attributes_t &cookie::attributes() const noexcept
{
	return m_attributes;
}

inline cookie::attributes_t &cookie::attributes() noexcept
{
	return m_attributes;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_COOKIE_H
