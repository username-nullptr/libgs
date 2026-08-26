
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_CONTAINER_HELPER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_CONTAINER_HELPER_H

#include <libgs/http/protocol/utils/core/body_norms.h>
#include <libgs/http/protocol/types.h>

namespace libgs::http
{

class type_helper
{
public:
	using value_t = value;
	using values_t = std::set<value_t>;

	using header_t = header;
	using headers_t = headers;

	using parameters_t = parameters;
};

template <typename Derived>
class LIBGS_HTTP_TAPI const_parameters : public type_helper
{
public:
	using derived_t = crtp_derived_t<Derived,const_parameters>;
	using container_t = headers_t;

public:
	explicit const_parameters(const parameters_t *parameters);
	virtual ~const_parameters() = default;

	[[nodiscard]] optional<value_t> parameter (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] bool contains_parameter (
		const core_concepts::text_p<char> auto &key,
		const value_t &value
	) const noexcept;

	[[nodiscard]] bool contains_parameter (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] optional<value_t> parameter(size_t index) const;
	[[nodiscard]] bool contains_parameter(size_t index) const noexcept;

	[[nodiscard]] const parameters_t &parameters() const noexcept;

protected:
	const parameters_t *m_parameters = nullptr;
};

template <typename Derived>
class LIBGS_HTTP_TAPI const_headers : public type_helper
{
public:
	using derived_t = crtp_derived_t<Derived,const_headers>;
	using container_t = headers_t;

public:
	explicit const_headers(const headers_t *headers);
	virtual ~const_headers() = default;

	[[nodiscard]] optional<value_t> header (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] bool contains_header (
		const core_concepts::text_p<char> auto &key,
		const value_t &value
	) const noexcept;

	[[nodiscard]] bool contains_header (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;

protected:
	const headers_t *m_headers = nullptr;
};

template <typename Cookie, typename Derived>
class LIBGS_HTTP_TAPI const_cookies : public type_helper
{
public:
	using derived_t = crtp_derived_t<Derived,const_cookies>;
	using cookie_t = Cookie;
	using cookies_t = map<cookie_t>;
	using container_t = cookies_t;

public:
	explicit const_cookies(const cookies_t *cookies);
	virtual ~const_cookies() = default;

	[[nodiscard]] optional<cookie_t> cookie (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] bool contains_cookie (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] const cookies_t &cookies() const noexcept;

protected:
	const cookies_t *m_cookies = nullptr;
};

template <typename Derived>
class LIBGS_HTTP_TAPI const_chunk_attributes : public type_helper
{
public:
	using derived_t = crtp_derived_t<Derived,const_chunk_attributes>;
	using container_t = values_t;

public:
	explicit const_chunk_attributes(const values_t *chunk_attributes);
	virtual ~const_chunk_attributes() = default;

	[[nodiscard]] bool contains_chunk_attribute(const value_t &attr) const noexcept;
	[[nodiscard]] const values_t &chunk_attributes() const noexcept;

protected:
	const values_t *m_chunk_attributes = nullptr;
};

template <typename Derived>
class LIBGS_HTTP_TAPI mutable_parameters : public const_parameters<Derived>
{
	using base_t = const_parameters<Derived>;

public:
	template <core_concepts::text_p<char> T>
	base_t::derived_t &set_parameter(T &&key, base_t::value_t value) noexcept;

	template <core_concepts::text_p<char> T>
	base_t::derived_t &unset_parameter(const T &key) noexcept;

	[[nodiscard]] base_t::parameters_t &parameters() noexcept;
	using base_t::parameters;
	using base_t::base_t;
};

template <typename Derived>
class LIBGS_HTTP_TAPI mutable_headers : public const_headers<Derived>
{
	using base_t = const_headers<Derived>;

public:
	template <core_concepts::text_p<char> T>
	base_t::derived_t &set_header(T &&key, base_t::value_t value) noexcept;

	template < core_concepts::text_p<char> T>
	base_t::derived_t &unset_header(const T &key) noexcept;

	[[nodiscard]] base_t::headers_t &headers() noexcept;
	using base_t::headers;
	using base_t::base_t;

public:
	template <typename T>
	static constexpr bool file_opt_token_v = concepts::file_opt_token_p <
		T, char, file_optype::combine, io_permission::read
	>;
	template <typename Opt>
	[[nodiscard]] auto set_header(Opt &&opt)
		noexcept requires file_opt_token_v<Opt>;
		// -> sys_expected<std::pair<body_norms_t,file_opt_token>>

	template <typename Opt>
	[[nodiscard]] auto make_file_opt_token(Opt &&opt)
		noexcept requires file_opt_token_v<Opt>;
};

template <typename Cookie, typename Derived>
class LIBGS_HTTP_TAPI mutable_cookies : public const_cookies<Cookie,Derived>
{
	using base_t = const_cookies<Cookie,Derived>;

public:
	template <core_concepts::text_p<char> T>
	base_t::derived_t &set_cookie(T &&key, base_t::cookie_t value) noexcept;

	template <core_concepts::text_p<char> T>
	base_t::derived_t &unset_cookie(const T &key) noexcept;

	[[nodiscard]] base_t::cookies_t &cookies() noexcept;
	using base_t::cookies;
	using base_t::base_t;
};

template <typename Derived>
class LIBGS_HTTP_TAPI mutable_chunk_attributes : public const_chunk_attributes<Derived>
{
	using base_t = const_chunk_attributes<Derived>;

public:
	base_t::derived_t &set_chunk_attribute(base_t::value_t attr) noexcept;
	base_t::derived_t &unset_chunk_attribute(const base_t::value_t &attr) noexcept;

	[[nodiscard]] base_t::values_t &chunk_attributes() noexcept;
	using base_t::chunk_attributes;
	using base_t::base_t;
};

} //namespace libgs::http
#include <libgs/http/protocol/utils/core/detail/container_helper.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_CONTAINER_HELPER_H
