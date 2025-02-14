
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H

namespace libgs::http
{

namespace detail
{

template <typename T>
struct response_helper_static_string;

#define LIBGS_HTTP_DETAIL_STATIC_STRING(_type, ...) \
	static constexpr const _type *colon                 = __VA_ARGS__##": "                             ; \
	static constexpr const _type *line_break            = __VA_ARGS__##"\r\n"                           ; \
	static constexpr const _type *bytes                 = __VA_ARGS__##"bytes"                          ; \
	static constexpr const _type *bytes_start           = __VA_ARGS__##"bytes="                         ; \
	static constexpr const _type *range_format          = __VA_ARGS__##"{}-{},"                         ; \
	static constexpr const _type *content_range_format  = __VA_ARGS__##"{}-{}/{}"                       ; \
	static constexpr const _type *content_type_boundary = __VA_ARGS__##"multipart/byteranges; boundary="; \

template <>
struct response_helper_static_string<char> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(char);
};

template <>
struct response_helper_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STATIC_STRING

} //namespace detail

template <core_concepts::char_type CharT>
class basic_response_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	struct string_pool :
		detail::string_pool<char_t>,
		detail::response_helper_static_string<char_t> {};

public:
	impl() = default;
	impl(version_t version, const headers_t &request_headers) :
		m_version(version), m_request_headers(&request_headers) {}

	[[nodiscard]] bool request_chunked() const
	{
		if( m_version < version::v11 )
			return false;

		auto it = m_request_headers->find(header_t::transfer_encoding);
		return it != m_request_headers->end() and
			str_to_lower(it->second.to_string()) == string_pool::chunked;
	}

public:
	const headers_t *m_request_headers;
	helper_t m_helper;

	version_t m_version = version::v11;
	status_t m_status = status::ok;

	cookies_t m_cookies {};
	string_t m_redirect_url {};
};

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::basic_response_helper(version_t version, const headers_t &request_headers) :
	m_impl(new impl(version, request_headers))
{

}

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::basic_response_helper(const headers_t &request_headers) :
	m_impl(new impl(detail::string_pool<char_t>::v_1_1, request_headers))
{

}

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::~basic_response_helper()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::basic_response_helper(basic_response_helper &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
	other.m_impl->m_request_headers = m_impl->m_request_headers;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::operator=(basic_response_helper &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	other.m_impl->m_request_headers = m_impl->m_request_headers;
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_status(status_t status)
{
	status_check(status);
	m_impl->m_status = status;
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_header(pair_init_t headers) noexcept
{
	set_map(m_impl->m_response_headers, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_cookie(cookie_init_t headers) noexcept
{
	set_map(m_impl->m_cookies, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attribute(attr_init_t attributes) noexcept
{
	set_set(m_impl->m_chunk_attributes, std::move(attributes));
	return *this;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_header(Args&&...args) noexcept
	requires concepts::set_key_attr_params<char_t,Args...>
{
	m_impl->m_helper.set_header(std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_cookie(Args&&...args) noexcept
	requires concepts::set_cookie_params<char_t,Args...>
{
	set_map(m_impl->m_cookies, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attribute(Args&&...args) noexcept
	requires concepts::set_attr_params<char_t,Args...>
{
	set_set(m_impl->m_chunk_attributes, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_redirect
(core_concepts::basic_string_type<char_t> auto &&url, redirect type)
{
	switch(type)
	{
#define X_MACRO(e,v) case redirect::e : set_status(v); break;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		default: throw runtime_error (
			"libgs::http::response_helper::redirect: Invalid redirect type: '{}'.", type
		);
	}
	return set_header(header_t::location, std::forward<decltype(url)>(url));
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::header_data(size_t body_size)
{
	using string_pool = typename impl::string_pool;
	if( m_impl->m_helper.state() != helper_t::state_t::header )
		return {};

	std::string buf;
	buf.reserve(4096);

	buf = std::format("HTTP/{} {} {}\r\n",
		version_string(m_impl->m_version), m_impl->m_status, status_description(m_impl->m_status)
	);
	m_impl->m_helper.unset_header(string_pool::set_cookie);

	if( m_impl->request_chunked() )
		m_impl->m_helper.set_header(header_t::transfer_encoding, string_pool::chunked);
	buf += m_impl->m_helper.header_data(body_size);

	for(auto &[ckey,cookie] : m_impl->m_cookies)
	{
		buf += "set-cookie: " + xxtombs(ckey) + "=" + xxtombs(cookie.value().to_string()) + ";";
		for(auto &[akey,attr] : cookie.attributes())
			buf += xxtombs(akey) + "=" + xxtombs(attr.to_string()) + ";";

		buf.pop_back();
		buf += "\r\n";
	}
	return buf + "\r\n";
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::body_data(const const_buffer &buffer)
{
	return m_impl->m_helper.body_data(buffer);
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::chunk_end_data(const map_helper_t &headers)
{
	return m_impl->chunk_end_data(headers);
}

template <core_concepts::char_type CharT>
version_t basic_response_helper<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <core_concepts::char_type CharT>
status_t basic_response_helper<CharT>::status() const noexcept
{
	return m_impl->m_status;
}

template <core_concepts::char_type CharT>
const typename basic_response_helper<CharT>::headers_t&
basic_response_helper<CharT>::headers() const noexcept
{
	return m_impl->m_response_headers;
}

template <core_concepts::char_type CharT>
const typename basic_response_helper<CharT>::cookies_t&
basic_response_helper<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <core_concepts::char_type CharT>
const typename basic_response_helper<CharT>::value_set_t&
basic_response_helper<CharT>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_header(Args&&...args) noexcept
	requires concepts::unset_pair_params<char_t,Args...>
{
	unset_map(m_impl->m_response_headers, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_cookie(Args&&...args) noexcept
	requires concepts::unset_pair_params<char_t,Args...>
{
	unset_map(m_impl->m_cookies, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_chunk_attribute(Args&&...args) noexcept
	requires concepts::unset_attr_params<char_t,Args...>
{
	unset_set(m_impl->m_chunk_attributes, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_header(key_init_t headers) noexcept
{
	unset_map(m_impl->m_response_headers, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::clear_header() noexcept
{
	m_impl->m_redirect_url.clear();
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_cookie(key_init_t headers) noexcept
{
	unset_map(m_impl->m_cookies, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::clear_cookie() noexcept
{
	m_impl->m_cookies.clear();
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_chunk_attribute(attr_init_t headers) noexcept
{
	unset_set(m_impl->m_chunk_attributes, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::clear_chunk_attribute() noexcept
{
	m_impl->m_chunk_attributes.clear();
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::reset() noexcept
{
	m_impl->m_version = version::v11;
	m_impl->m_status = status::ok;

	m_impl->m_cookies.clear();
	m_impl->m_redirect_url.clear();

	m_impl->m_helper.reset();
	return *this;
}

template <core_concepts::char_type CharT>
typename basic_response_helper<CharT>::pro_state_t basic_response_helper<CharT>::pro_state() const noexcept
{
	return m_impl->m_helper.state();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
