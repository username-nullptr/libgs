
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

template <concept_char_type CharT>
basic_cookie<CharT>::basic_cookie()
{
	if constexpr( is_char_v<CharT> )
		m_attributes[attributes_type::path] = "/";
	else
		m_attributes[attributes_type::path] = L"/";
}

template <concept_char_type CharT>
basic_cookie<CharT>::basic_cookie(value_type &v) :
	m_value(std::move(v))
{

}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_value(value_type v) noexcept
{
	m_value = std::move(v);
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::operator=(value_type v) noexcept
{
	m_value = std::move(v);
}

template <concept_char_type CharT>
const basic_value<CharT> &basic_cookie<CharT>::value() const noexcept
{
	return m_value;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_cookie<CharT>::value() noexcept
{
	return m_value;
}

template <concept_char_type CharT>
basic_cookie<CharT>::operator const value_type&() const noexcept
{
	return m_value;
}

template <concept_char_type CharT>
basic_cookie<CharT>::operator value_type&() noexcept
{
	return m_value;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::domain() const
{
	auto it = m_attributes.find(attributes_type::domain);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::domain: key 'Domain' not exists.");
	return it->second.to_string();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::path() const
{
	auto it = m_attributes.find(attributes_type::path);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::path: key 'Path' not exists.");
	return it->second.to_string();
}

template <concept_char_type CharT>
size_t basic_cookie<CharT>::size() const
{
	auto it = m_attributes.find(attributes_type::size);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::cookies_size: key 'Size' not exists.");
	return it->second.template get<size_t>();
}

template <concept_char_type CharT>
uint64_t basic_cookie<CharT>::expires() const
{
	auto it = m_attributes.find(attributes_type::expires);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::expires: key 'Expires' not exists.");
	return it->second.template get<uint64_t>();
}

template <concept_char_type CharT>
uint64_t basic_cookie<CharT>::max_age() const
{
	auto it = m_attributes.find(attributes_type::max_age);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::max_age: key 'Max-age' not exists.");
	return it->second.template get<uint64_t>();
}

template <concept_char_type CharT>
bool basic_cookie<CharT>::http_only() const
{
	auto it = m_attributes.find(attributes_type::http_only);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::http_only: key 'HttpOnly' not exists.");
	return it->second.to_bool();
}

template <concept_char_type CharT>
bool basic_cookie<CharT>::secure() const
{
	auto it = m_attributes.find(attributes_type::secure);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::secure: key 'Secure' not exists.");
	return it->second.to_bool();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::same_site() const
{
	auto it = m_attributes.find(attributes_type::same_site);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::same_site: key 'SameSite' not exists.");
	return it->second.to_string();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::priority() const
{
	auto it = m_attributes.find(attributes_type::priority);
	if( it == m_attributes.end() )
		throw runtime_error("libgs::http::cookie::priority: key 'Priority' not exists.");
	return it->second.to_string();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::domain_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::domain);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::path_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::path);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <concept_char_type CharT>
size_t basic_cookie<CharT>::size_or(size_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::size);
	return it == m_attributes.end() ? default_value : it->second.template get<size_t>();
}

template <concept_char_type CharT>
uint64_t basic_cookie<CharT>::expires_or(uint64_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::expires);
	return it == m_attributes.end() ? default_value : it->second.template get<uint64_t>();
}

template <concept_char_type CharT>
uint64_t basic_cookie<CharT>::max_age_or(uint64_t default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::max_age);
	return it == m_attributes.end() ? default_value : it->second.template get<uint64_t>();
}

template <concept_char_type CharT>
bool basic_cookie<CharT>::http_only_or(bool default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::http_only);
	return it == m_attributes.end() ? default_value : it->second.to_bool();
}

template <concept_char_type CharT>
bool basic_cookie<CharT>::secure_or(bool default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::secure);
	return it == m_attributes.end() ? default_value : it->second.to_bool();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::same_site_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::same_site);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cookie<CharT>::priority_or(std::basic_string<CharT> default_value) const noexcept
{
	auto it = m_attributes.find(attributes_type::priority);
	return it == m_attributes.end() ? default_value : it->second.to_string();
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_domain(str_type domain)
{
	m_attributes[attributes_type::domain] = std::move(domain);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_path(str_type path)
{
	m_attributes[attributes_type::path] = std::move(path);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_size(size_t size)
{
	m_attributes[attributes_type::size] = size;
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_expires(uint64_t seconds)
{
	m_attributes[attributes_type::expires] = seconds;
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_max_age(uint64_t seconds)
{
	m_attributes[attributes_type::max_age] = seconds;
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_http_only(bool flag)
{
	m_attributes[attributes_type::http_only] = flag;
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_secure(bool flag)
{
	m_attributes[attributes_type::secure] = flag;
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_same_site(str_type sst)
{
	m_attributes[attributes_type::same_site] = std::move(sst);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_priority(str_type pt)
{
	m_attributes[attributes_type::priority] = std::move(pt);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_domain()
{
	m_attributes.erase(attributes_type::domain);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_path()
{
	m_attributes.erase(attributes_type::path);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_size()
{
	m_attributes.erase(attributes_type::size);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_expires()
{
	m_attributes.erase(attributes_type::expires);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_max_age()
{
	m_attributes.erase(attributes_type::max_age);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_http_only()
{
	m_attributes.erase(attributes_type::http_only);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_secure()
{
	m_attributes.erase(attributes_type::secure);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_same_site()
{
	m_attributes.erase(attributes_type::same_site);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_priority()
{
	m_attributes.erase(attributes_type::priority);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> basic_cookie<CharT>::attributes(const str_type &key) const
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

template <concept_char_type CharT>
basic_value<CharT> basic_cookie<CharT>::attributes_or(const str_type &key, value_type default_value) const noexcept
{
	auto it = m_attributes.find(key);
	return it == m_attributes.end() ? default_value : it->second;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::set_attribute(str_type key, value_type v)
{
	m_attributes[std::move(key)] = std::move(v);
	return *this;
}

template <concept_char_type CharT>
basic_cookie<CharT> &basic_cookie<CharT>::unset_attribute(const str_type &key)
{
	m_attributes.erase(key);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
T basic_cookie<CharT>::attributes(const str_type &key) const
	requires std::is_arithmetic_v<T> or std::is_same_v<T,str_type>
{
	return attributes().template get<T>();
}

template <concept_char_type CharT>
template <typename T>
T basic_cookie<CharT>::attributes_or(const str_type &key, T default_value) const noexcept
	requires std::is_arithmetic_v<T> or std::is_same_v<T,str_type>
{
	return attributes_or().template get<T>();
}

template <concept_char_type CharT>
basic_cookie_attributes<CharT> basic_cookie<CharT>::attributes() const noexcept
{
	return m_attributes;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_DETAIL_COOKIE_H
