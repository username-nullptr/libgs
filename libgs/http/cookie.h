
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

#define LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY \
	X_MACRO( domain    , "Domain"   ) \
	X_MACRO( path      , "Path"     ) \
	X_MACRO( size      , "Size"     ) \
	X_MACRO( expires   , "Expires"  ) \
	X_MACRO( max_age   , "Max-Age"  ) \
	X_MACRO( http_only , "HttpOnly" ) \
	X_MACRO( secure    , "Secure"   ) \
	X_MACRO( same_site , "SameSite" ) \
	X_MACRO( priority  , "Priority" )

template <> struct basic_cookie_attribute<char>
{
#define X_MACRO(n,s) static constexpr const char *n = s;
	LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY
#undef X_MACRO
};

template <> struct basic_cookie_attribute<wchar_t>
{
#define X_MACRO(n,s) static constexpr const wchar_t *n = L##s;
	LIBGS_HTTP_COOKEI_ATTRUBUTE_KEY
#undef X_MACRO
};

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
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using value_t = basic_value<char_t>;

	using attribute_t = basic_cookie_attribute<char_t>;
	using attributes_t = basic_cookie_attributes<char_t>;

	using pair_init_t = basic_key_attr_init<char_t>;
	using key_init_t = basic_key_init<char_t>;

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

template <core_concepts::char_type CharT>
using basic_cookie_init = basic_pair_init<CharT,basic_cookie<CharT>>;

using cookie_init  = basic_cookie_init<char>;
using wcookie_init = basic_cookie_init<wchar_t>;

namespace concepts
{

template <typename CharT, typename...Args>
concept set_cookie_params = set_pair_params <
	CharT, basic_cookie<CharT>, Args...
>;

}} //namespace libgs::http::concepts
#include <libgs/http/detail/cookie.h>


#endif //LIBGS_HTTP_COOKIE_H
