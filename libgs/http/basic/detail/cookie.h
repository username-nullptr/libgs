
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_COOKIE_H
#define LIBGS_HTTP_BASIC_DETAIL_COOKIE_H

namespace libgs::http
{

template <core_concepts::char_type CharT>
basic_cookie<CharT>::basic_cookie()
{
	if constexpr( is_char_v<CharT> )
		m_attributes[attribute_t::path] = "/";
	else
		m_attributes[attribute_t::path] = L"/";
}

template <core_concepts::char_type CharT>
basic_cookie<CharT>::basic_cookie(value_t v) :
	m_value(std::move(v))
{

}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_value(value_t v) noexcept
{
	m_value = std::move(v);
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::operator=(value_t v) noexcept
{
	m_value = std::move(v);
}

template <core_concepts::char_type CharT>
const basic_value<CharT> &basic_cookie<CharT>::value() const noexcept
{
	return m_value;
}

template <core_concepts::char_type CharT>
basic_value<CharT> &basic_cookie<CharT>::value() noexcept
{
	return m_value;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT>::operator const value_t&() const noexcept
{
	return m_value;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT>::operator value_t&() noexcept
{
	return m_value;
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::domain() const
{
	auto it = m_attributes.find(attributes_t::domain);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::domain: key 'Domain' not exists.");
	return it->second.to_string();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::path() const
{
	auto it = m_attributes.find(attributes_t::path);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::path: key 'Path' not exists.");
	return it->second.to_string();
}

template <core_concepts::char_type CharT>
size_t basic_cookie<CharT>::size() const
{
	auto it = m_attributes.find(attributes_t::size);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::cookies_size: key 'Size' not exists.");
	return it->second.template get<size_t>();
}

template <core_concepts::char_type CharT>
uint64_t basic_cookie<CharT>::expires() const
{
	auto it = m_attributes.find(attributes_t::expires);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::expires: key 'Expires' not exists.");
	return it->second.template get<uint64_t>();
}

template <core_concepts::char_type CharT>
uint64_t basic_cookie<CharT>::max_age() const
{
	auto it = m_attributes.find(attributes_t::max_age);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::max_age: key 'Max-age' not exists.");
	return it->second.template get<uint64_t>();
}

template <core_concepts::char_type CharT>
bool basic_cookie<CharT>::http_only() const
{
	auto it = m_attributes.find(attributes_t::http_only);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::http_only: key 'HttpOnly' not exists.");
	return it->second.to_bool();
}

template <core_concepts::char_type CharT>
bool basic_cookie<CharT>::secure() const
{
	auto it = m_attributes.find(attributes_t::secure);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::secure: key 'Secure' not exists.");
	return it->second.to_bool();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::same_site() const
{
	auto it = m_attributes.find(attributes_t::same_site);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::same_site: key 'SameSite' not exists.");
	return it->second.to_string();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::priority() const
{
	auto it = m_attributes.find(attributes_t::priority);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::priority: key 'Priority' not exists.");
	return it->second.to_string();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::domain_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::domain);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::path_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::path);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <core_concepts::char_type CharT>
size_t basic_cookie<CharT>::size_or(size_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::size);
	return it == m_attributes.end() ? default_value : it->second.template get<size_t>();
}

template <core_concepts::char_type CharT>
uint64_t basic_cookie<CharT>::expires_or(uint64_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::expires);
	return it == m_attributes.end() ? default_value : it->second.template get<uint64_t>();
}

template <core_concepts::char_type CharT>
uint64_t basic_cookie<CharT>::max_age_or(uint64_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::max_age);
	return it == m_attributes.end() ? default_value : it->second.template get<uint64_t>();
}

template <core_concepts::char_type CharT>
bool basic_cookie<CharT>::http_only_or(bool default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::http_only);
	return it == m_attributes.end() ? default_value : it->second.to_bool();
}

template <core_concepts::char_type CharT>
bool basic_cookie<CharT>::secure_or(bool default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::secure);
	return it == m_attributes.end() ? default_value : it->second.to_bool();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::same_site_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::same_site);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::priority_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_t::priority);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_domain(string_t domain)
{
	m_attributes[attributes_t::domain] = std::move(domain);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_path(string_t path)
{
	m_attributes[attributes_t::path] = std::move(path);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_size(size_t size)
{
	m_attributes[attributes_t::size] = size;
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_expires(uint64_t seconds)
{
	m_attributes[attributes_t::expires] = seconds;
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_max_age(uint64_t seconds)
{
	m_attributes[attributes_t::max_age] = seconds;
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_http_only(bool flag)
{
	m_attributes[attributes_t::http_only] = flag;
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_secure(bool flag)
{
	m_attributes[attributes_t::secure] = flag;
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_same_site(string_t sst)
{
	m_attributes[attributes_t::same_site] = std::move(sst);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_priority(string_t pt)
{
	m_attributes[attributes_t::priority] = std::move(pt);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_domain()
{
	m_attributes.erase(attributes_t::domain);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_path()
{
	m_attributes.erase(attributes_t::path);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_size()
{
	m_attributes.erase(attributes_t::size);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_expires()
{
	m_attributes.erase(attributes_t::expires);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_max_age()
{
	m_attributes.erase(attributes_t::max_age);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_http_only()
{
	m_attributes.erase(attributes_t::http_only);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_secure()
{
	m_attributes.erase(attributes_t::secure);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_same_site()
{
	m_attributes.erase(attributes_t::same_site);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_priority()
{
	m_attributes.erase(attributes_t::priority);
	return *this;
}

template <core_concepts::char_type CharT>
basic_value<CharT> basic_cookie<CharT>::attributes(const string_t &key) const
{
	auto it = m_attributes.find(key);
	if( it == m_attributes.end() )
	{
		if constexpr( is_char_v<CharT> )
			throw runtime_error("libgs::http::cookie::attributes: key '{}' not exists.", key);
		else
			throw runtime_error("libgs::http::cookie::attributes: key '{}' not exists.", wcstombs(key));
	}
	return it->second;
}

template <core_concepts::char_type CharT>
basic_value<CharT> basic_cookie<CharT>::attributes_or(const string_t &key, value_t default_value) const noexcept
{
	auto it = m_attributes.find(key);
	return it == m_attributes.end() ? default_value : it->second;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_attribute(string_t key, value_t v)
{
	m_attributes[std::move(key)] = std::move(v);
	return *this;
}

template <core_concepts::char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_attribute(const string_t &key)
{
	m_attributes.erase(key);
	return *this;
}

template <core_concepts::char_type CharT>
template <typename T>
T basic_cookie<CharT>::attributes(const string_t &key) const
	requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>
{
	return attributes().template get<T>();
}

template <core_concepts::char_type CharT>
template <typename T>
T basic_cookie<CharT>::attributes_or(const string_t &key, T default_value) const noexcept
	requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>
{
	return attributes_or().template get<T>();
}

template <core_concepts::char_type CharT>
basic_cookie_attributes<CharT> basic_cookie<CharT>::attributes() const noexcept
{
	return m_attributes;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_DETAIL_COOKIE_H
