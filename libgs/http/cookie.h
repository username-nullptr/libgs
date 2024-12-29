
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

template <core_concepts::char_type CharT>
struct basic_cookie_attribute;

#define LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY(_type, ...) \
	static constexpr const _type *domain    = __VA_ARGS__##"Domain"; \
	static constexpr const _type *path      = __VA_ARGS__##"Path"; \
	static constexpr const _type *size      = __VA_ARGS__##"Size"; \
	static constexpr const _type *expires   = __VA_ARGS__##"Expires"; \
	static constexpr const _type *max_age   = __VA_ARGS__##"Max-Age"; \
	static constexpr const _type *http_only = __VA_ARGS__##"HttpOnly"; \
	static constexpr const _type *secure    = __VA_ARGS__##"Secure"; \
	static constexpr const _type *same_site = __VA_ARGS__##"SameSite"; \
	static constexpr const _type *priority  = __VA_ARGS__##"Priority"

template <> struct basic_cookie_attribute<char> { LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY(char); };
template <> struct basic_cookie_attribute<wchar_t> { LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY(wchar_t,L); };

using cookie_attribute = basic_cookie_attribute<char>;
using wcookie_attribute = basic_cookie_attribute<wchar_t>;

template <core_concepts::char_type CharT>
using basic_cookie_attributes = std::map <
	std::basic_string<CharT>,
	basic_value<CharT>,
	basic_less_case_insensitive<CharT>
>;

using cookie_attributes = basic_cookie_attributes<char>;
using wcookie_attributes = basic_cookie_attributes<wchar_t>;

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_cookie
{
public:
	using string_t = std::basic_string<CharT>;
	using value_t = basic_value<CharT>;
	using attribute_t = basic_cookie_attribute<CharT>;
	using attributes_t = basic_cookie_attributes<CharT>;

public:
	basic_cookie();
	basic_cookie(value_t v);

	basic_cookie(const basic_cookie &other) = default;
	basic_cookie &operator=(const basic_cookie &other) = default;
	basic_cookie(basic_cookie &&other) noexcept = default;
	basic_cookie &operator=(basic_cookie &&other) noexcept = default;
	virtual ~basic_cookie() = default;

public:
	basic_cookie &set_value(value_t v) noexcept;
	basic_cookie &operator=(value_t v) noexcept;

public:
	[[nodiscard]] const value_t &value() const noexcept;
	[[nodiscard]] value_t &value() noexcept;

	operator const value_t&() const noexcept;
	operator value_t&() noexcept;

public:
	[[nodiscard]] string_t domain() const;
	[[nodiscard]] string_t path() const;
	[[nodiscard]] size_t size() const;

	[[nodiscard]] uint64_t expires() const;
	[[nodiscard]] uint64_t max_age() const;

	[[nodiscard]] bool http_only() const;
	[[nodiscard]] bool secure() const;

	[[nodiscard]] string_t same_site() const;
	[[nodiscard]] string_t priority() const;

public:
	[[nodiscard]] string_t domain_or(string_t default_value = {}) const noexcept;
	[[nodiscard]] string_t path_or(string_t default_value = {}) const noexcept;
	[[nodiscard]] size_t size_or(size_t default_value = 0) const noexcept;

	[[nodiscard]] uint64_t expires_or(uint64_t default_value = 0) const noexcept;
	[[nodiscard]] uint64_t max_age_or(uint64_t default_value = 0) const noexcept;

	[[nodiscard]] bool http_only_or(bool default_value = false) const noexcept;
	[[nodiscard]] bool secure_or(bool default_value = false) const noexcept;

	[[nodiscard]] string_t same_site_or(string_t default_value = {}) const noexcept;
	[[nodiscard]] string_t priority_or(string_t default_value = {}) const noexcept;

public:
	basic_cookie &set_domain(string_t domain);
	basic_cookie &set_path(string_t path);
	basic_cookie &set_size(size_t size);

	basic_cookie &set_expires(uint64_t seconds);
	basic_cookie &set_max_age(uint64_t seconds);

	basic_cookie &set_http_only(bool flag);
	basic_cookie &set_secure(bool flag);

	basic_cookie &set_same_site(string_t sst);
	basic_cookie &set_priority(string_t pt);

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
	[[nodiscard]] value_t attributes(const string_t &key) const;
	[[nodiscard]] value_t attributes_or(const string_t &key, value_t default_value = {}) const noexcept;
	basic_cookie &set_attribute(string_t key, value_t v);
	basic_cookie &unset_attribute(const string_t &key);

	template <typename T>
	[[nodiscard]] T attributes(const string_t &key) const
		requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>;

	template <typename T>
	[[nodiscard]] T attributes_or(const string_t &key, T default_value = {}) const noexcept
		requires std::is_arithmetic_v<T> or std::is_same_v<T,string_t>;

	[[nodiscard]] attributes_t attributes() const noexcept;

protected:
	value_t m_value;
	attributes_t m_attributes;
};

using cookie = basic_cookie<char>;
using wcookie = basic_cookie<wchar_t>;

template <core_concepts::char_type CharT>
using basic_cookie_values = std::map <
	std::basic_string<CharT>,
	basic_value<CharT>,
	basic_less_case_insensitive<CharT>
>;

using cookie_values = basic_cookie_values<char>;
using wcookie_values = basic_cookie_values<wchar_t>;

template <core_concepts::char_type CharT>
using basic_cookies = std::map <
	std::basic_string<CharT>,
	basic_cookie<CharT>,
	basic_less_case_insensitive<CharT>
>;

using cookies = basic_cookies<char>;
using wcookies = basic_cookies<wchar_t>;

} //namespace libgs::http
#include <libgs/http/detail/cookie.h>


#endif //LIBGS_HTTP_COOKIE_H
