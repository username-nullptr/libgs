
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_COOKIE_H
#define LIBGS_HTTP_NT_PROTOCOL_COOKIE_H

#include <libgs/http_nt/global.h>
#include <libgs/core/value.h>

namespace libgs::http_nt
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

class LIBGS_HTTP_NT_API cookie final
{
public:
	using value_t = libgs::value;
	using attributes_t = value_map;

public:
	cookie();
	cookie(value_t value);
	~cookie();

	cookie(const cookie &other);
	cookie &operator=(const cookie &other);

	cookie(cookie &&other) noexcept;
	cookie &operator=(cookie &&other) noexcept;

public:
	cookie &set_value(value_t value) noexcept;
	cookie &operator=(value_t v) noexcept;

	template <typename T>
	[[nodiscard]] decltype(auto) value() requires
		core_concepts::value_get<T,char>;

	[[nodiscard]] value_t value() const noexcept;
	operator value_t() const noexcept;

public:
	[[nodiscard]] optional<std::string> domain() const noexcept;
	[[nodiscard]] optional<std::string> path() const noexcept;

	[[nodiscard]] optional<std::string> same_site() const noexcept;
	[[nodiscard]] optional<std::string> priority() const noexcept;

	[[nodiscard]] optional<uint64_t> expires() const noexcept;
	[[nodiscard]] optional<uint64_t> max_age() const noexcept;
	[[nodiscard]] optional<size_t> size() const noexcept;

	[[nodiscard]] optional<bool> http_only() const noexcept;
	[[nodiscard]] optional<bool> secure() const noexcept;

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
	cookie &set_attribute(core_concepts::text_p<char> auto &&key, value_t attr) noexcept;
	cookie &unset_attribute(const core_concepts::text_p<char> auto &key) noexcept;

	[[nodiscard]] optional<value_t> attribute(const core_concepts::text_p<char> auto &key) noexcept;
	[[nodiscard]] const attributes_t &attributes() const noexcept;
	[[nodiscard]] attributes_t &attributes() noexcept;

private:
	class impl;
	impl *m_impl;
};

using cookie_attributes = cookie::attributes_t;
using cookie_values = value_map;
using cookies = map<cookie>;

} //namespace libgs::http_nt::concepts
#include <libgs/http_nt/protocol/detail/cookie.h>


#endif //LIBGS_HTTP_NT_PROTOCOL_COOKIE_H
