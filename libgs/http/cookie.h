
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
#include <libgs/core/value.h>

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

class LIBGS_HTTP_VAPI cookie
{
public:
	using value_t = libgs::value;
	using string_t = std::string;
	using attributes_t = map<value_t>;

public:
	cookie();
	cookie(value_t value);
	virtual ~cookie() = default;

	cookie(const cookie &other) = default;
	cookie &operator=(const cookie &other) = default;

	cookie(cookie &&other) noexcept = default;
	cookie &operator=(cookie &&other) noexcept = default;

public:
	cookie &set_value(value_t value) noexcept;
	cookie &operator=(value_t v) noexcept;

	template <core_concepts::value_get<char> T = value_t>
	[[nodiscard]] T value() noexcept;

	[[nodiscard]] value_t value() noexcept;
	operator value_t() noexcept;

public:
	[[nodiscard]] string_t domain() const;
	[[nodiscard]] string_t path() const;

	[[nodiscard]] string_t same_site() const;
	[[nodiscard]] string_t priority() const;

	[[nodiscard]] uint64_t expires() const;
	[[nodiscard]] uint64_t max_age() const;
	[[nodiscard]] size_t size() const;

	[[nodiscard]] bool http_only() const;
	[[nodiscard]] bool secure() const;

public:
	[[nodiscard]] string_t domain_or(const value_t &def_value = {}) const noexcept;
	[[nodiscard]] string_t domain_or(value_t &&def_value = {}) const noexcept;

	[[nodiscard]] string_t path_or(const value_t &def_value = {}) const noexcept;
	[[nodiscard]] string_t path_or(value_t &&def_value = {}) const noexcept;

	[[nodiscard]] string_t same_site_or(const value_t &def_value = {}) const noexcept;
	[[nodiscard]] string_t same_site_or(value_t &&def_value = {}) const noexcept;

	[[nodiscard]] string_t priority_or(const value_t &def_value = {}) const noexcept;
	[[nodiscard]] string_t priority_or(value_t &&def_value = {}) const noexcept;

	[[nodiscard]] uint64_t expires_or(uint64_t def_value = 0) const noexcept;
	[[nodiscard]] uint64_t max_age_or(uint64_t def_value = 0) const noexcept;
	[[nodiscard]] size_t size_or(size_t def_value = 0) const noexcept;

	[[nodiscard]] bool http_only_or(bool def_value = false) const noexcept;
	[[nodiscard]] bool secure_or(bool def_value = false) const noexcept;

public:
	cookie &set_domain(value_t domain);
	cookie &set_path(value_t path);

	cookie &set_same_site(value_t sst);
	cookie &set_priority(value_t pt);

	cookie &set_expires(uint64_t seconds);
	cookie &set_max_age(uint64_t seconds);
	cookie &set_size(size_t size);

	cookie &set_http_only(bool flag);
	cookie &set_secure(bool flag);

public:
	cookie &unset_domain();
	cookie &unset_path();

	cookie &unset_same_site();
	cookie &unset_priority();

	cookie &unset_expires();
	cookie &unset_max_age();
	cookie &unset_size();

	cookie &unset_http_only();
	cookie &unset_secure();

public:
	template <core_concepts::value_get<char> T = value_t>
	[[nodiscard]] T attribute(const core_concepts::text_p<char> auto &key) const;

	template <core_concepts::text_arg_p<char> T = value_t>
	[[nodiscard]] decltype(auto) attribute_or (
		const core_concepts::text_p<char> auto &key, T &&def_value = {}
	) const noexcept;

public:
	cookie &set_attribute(core_concepts::text_p<char> auto &&key, value_t attr) noexcept;
	cookie &unset_attribute(const core_concepts::text_p<char> auto &key) noexcept;

	[[nodiscard]] const attributes_t &attributes() const noexcept;
	[[nodiscard]] attributes_t &attributes() noexcept;

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
