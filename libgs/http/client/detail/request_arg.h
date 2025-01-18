
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_ARG_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_ARG_H

#include <libgs/core/string_list.h>

namespace libgs::http
{

namespace detail
{

template <core_concepts::char_type T>
struct _client_client_request_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *ampersand  = __VA_ARGS__##"&"; \
	static constexpr const _type *assignment = __VA_ARGS__##"=";

template <>
struct _client_client_request_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _client_client_request_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_request_arg<CharT>::impl
{
	using string_list_t = basic_string_list<char_t>;

	struct string_pool :
		detail::_client_client_request_static_string<char_t>,
		detail::string_pool<char_t> {};

public:
	explicit impl(url_t &&url) :
		m_url(std::move(url)) {}

	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	url_t m_url;
	headers_t m_headers {
		{ header_t::content_type, string_pool::text_plain }
	};
	cookies_t m_cookies {};
	value_list_t m_chunk_attributes {};
};

template <core_concepts::char_type CharT>
basic_request_arg<CharT>::basic_request_arg(url_t url) :
	m_impl(new impl(std::move(url)))
{

}

template <core_concepts::char_type CharT>
basic_request_arg<CharT>::basic_request_arg() :
	basic_request_arg(url_t())
{

}

template <core_concepts::char_type CharT>
basic_request_arg<CharT>::~basic_request_arg()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT>::basic_request_arg(const basic_request_arg &other) noexcept :
	m_impl(new impl(*other.m_impl))
{

}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::operator=(const basic_request_arg &other) noexcept
{
	*m_impl = *other.m_impl;
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT>::basic_request_arg(basic_request_arg &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl({});
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::operator=(basic_request_arg &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl({});
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::set_url(url_t url)
{
	m_impl->m_url = std::move(url);
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::set_header(string_view_t key, value_t value) noexcept
{
	m_impl->m_headers[str_to_lower(key)] = std::move(value);
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::set_cookie(string_view_t key, value_t value) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(value);
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::set_chunk_attribute(value_t attribute)
{
	set_header(basic_header<CharT>::transfer_encoding, detail::string_pool<char_t>::chunked);
	m_impl->m_chunk_attributes.emplace_back(std::move(attribute));
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::set_chunk_attributes(value_list_t attributes)
{
	set_header(basic_header<CharT>::transfer_encoding, detail::string_pool<char_t>::chunked);
	for(auto &value : attributes)
		m_impl->m_chunk_attributes.emplace_back(std::move(value));
	return *this;
}

template <core_concepts::char_type CharT>
const basic_headers<CharT> &basic_request_arg<CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <core_concepts::char_type CharT>
const basic_cookie_values<CharT> &basic_request_arg<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <core_concepts::char_type CharT>
const basic_value_list<CharT> &basic_request_arg<CharT>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <core_concepts::char_type CharT>
const basic_url<CharT> &basic_request_arg<CharT>::url() const noexcept
{
	return m_impl->m_url;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_request_arg<CharT>::url() noexcept
{
	return m_impl->m_url;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::unset_header(string_view_t key)
{
	m_impl->m_headers.erase({key.data(), key.size()});
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_cookies.erase({key.data(), key.size()});
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::unset_chunk_attribute(const value_t &attributes)
{
	m_impl->m_chunk_attributes.erase(attributes);
	return *this;
}

template <core_concepts::char_type CharT>
basic_request_arg<CharT> &basic_request_arg<CharT>::reset()
{
	using string_pool = detail::string_pool<char_t>;
	m_impl->m_path = string_pool::root;
	m_impl->m_chunk_attributes.clear();
	m_impl->m_cookies.clear();
	m_impl->m_headers = {
		{ header_t::content_type, string_pool::text_plain }
	};
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_ARG_H