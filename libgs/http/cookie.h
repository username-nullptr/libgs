
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

#ifndef LIBGS_HTTP_COOKIE_H
#define LIBGS_HTTP_COOKIE_H

#include <libgs/http/global.h>

namespace libgs::http
{

struct cookie_attribute
{
static constexpr const char
	*domain    = "Domain"  ,
	*path      = "Path"    ,
	*size      = "Size"    ,
	*expires   = "Expires" ,
	*max_age   = "Max-Age" ,
	*http_only = "HttpOnly",
	*secure    = "Secure"  ,
	*same_site = "SameSite",
	*priority  = "Priority";
};

class LIBGS_HTTP_TAPI cookie
{
public:
	using attributes_t = map<value>;

public:
	cookie();
	cookie(value_t v);
	virtual ~cookie() = default;

	cookie(const cookie &other) = default;
	cookie &operator=(const cookie &other) = default;

	cookie(cookie &&other) noexcept = default;
	cookie &operator=(cookie &&other) noexcept = default;

public:
	cookie &set_value(value_t v) noexcept;
	cookie &operator=(value_t v) noexcept;

public:
	[[nodiscard]] const value_t &value() const noexcept;
	[[nodiscard]] value_t &value() noexcept;

	operator const value_t&() const noexcept;
	operator value_t&() noexcept;

public:
	template <typename...Args>
	attributes_t &attributes(Args&&...args) requires
		core_concepts::constructible<attributes_t,Args...>;

	[[nodiscard]] const attributes_t &attributes() const noexcept;
	[[nodiscard]] attributes_t &attributes() noexcept;


public:
	[[nodiscard]] std::string_view domain() const;
	[[nodiscard]] std::string_view path() const;
	[[nodiscard]] size_t size() const;

	[[nodiscard]] uint64_t expires() const;
	[[nodiscard]] uint64_t max_age() const;

	[[nodiscard]] bool http_only() const;
	[[nodiscard]] bool secure() const;

	[[nodiscard]] std::string_view same_site() const;
	[[nodiscard]] std::string_view priority() const;

public:
	[[nodiscard]] std::string_view domain_or(std::string_view default_value = {}) const noexcept;
	[[nodiscard]] std::string_view path_or(std::string_view default_value = {}) const noexcept;
	[[nodiscard]] size_t size_or(size_t default_value = 0) const noexcept;

	[[nodiscard]] uint64_t expires_or(uint64_t default_value = 0) const noexcept;
	[[nodiscard]] uint64_t max_age_or(uint64_t default_value = 0) const noexcept;

	[[nodiscard]] bool http_only_or(bool default_value = false) const noexcept;
	[[nodiscard]] bool secure_or(bool default_value = false) const noexcept;

	[[nodiscard]] std::string_view same_site_or(std::string_view default_value = {}) const noexcept;
	[[nodiscard]] std::string_view priority_or(std::string_view default_value = {}) const noexcept;

public:
	cookie &set_domain(string_t domain);
	cookie &set_path(string_t path);
	cookie &set_size(size_t size);

	cookie &set_expires(uint64_t seconds);
	cookie &set_max_age(uint64_t seconds);

	cookie &set_http_only(bool flag);
	cookie &set_secure(bool flag);

	cookie &set_same_site(string_t sst);
	cookie &set_priority(string_t pt);

public:
	basic_cookie &unset_domain();
	basic_cookie &unset_path();
	basic_cookie &unset_size();

	basic_cookie &unset_expires();
	basic_cookie &unset_max_age();

	basic_cookie &unset_http_only();
	basic_cookie &unset_secure();

	basic_cookie &unset_same_site();
	basic_cookie &unset_priority();

public:
	template <typename...Args>
	basic_cookie &set_attribute(Args&&...args) noexcept requires
		concepts::set_key_attr_params<char_t,Args...>;

	template <typename...Args>
	basic_cookie &unset_attribute(Args&&...args) noexcept requires
		concepts::unset_pair_params<char_t,Args...>;

	basic_cookie &set_attribute(pair_init_t init) noexcept;
	basic_cookie &unset_attribute(key_init_t init) noexcept;

public:
	template <typename T>
	[[nodiscard]] T attribute(const string_t &key) const
		requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>;

	template <typename T>
	[[nodiscard]] T attribute_or(const string_t &key, T default_value = {}) const noexcept
		requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>;

	[[nodiscard]] value_t attribute(const string_t &key) const;
	[[nodiscard]] value_t attribute_or(const string_t &key, value_t default_value = {}) const noexcept;
	[[nodiscard]] attributes_t attributes() const noexcept;

protected:
	value_t m_value;
	attributes_t m_attributes;
};

using cookie_attributes = cookie::attributes_t;

template <core_concepts::character CharT>
using cookie_values = map<value>;

template <core_concepts::character CharT>
using cookies = map<cookie>;

} //namespace libgs::http::concepts
#include <libgs/http/detail/cookie.h>


#endif //LIBGS_HTTP_COOKIE_H
