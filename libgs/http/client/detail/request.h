
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H

#include <libgs/http/client/session_pool.h>

namespace libgs::http
{

namespace detail
{

template <concept_char_type T>
struct _client_request_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *ampersand  = __VA_ARGS__##"&"; \
	static constexpr const _type *assignment = __VA_ARGS__##"=";

template <>
struct _client_request_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _client_request_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <concept_stream_requires Stream, concept_char_type CharT>
class basic_client_request<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	using string_list_t = basic_string_list<CharT>;
	using session_pool_t = basic_session_pool<Stream>;

	struct string_pool :
		detail::_client_request_static_string<CharT>,
		detail::string_pool<CharT> {};

public:
	explicit impl(url_t &&url) :
		m_url(std::move(url)) {}

public:
	url_t m_url;
	session_pool_t m_session_pool;

	http::method m_method = http::method::GET;
	headers_t m_headers {
		{ header_t::content_type, string_pool::text_plain }
	};
	cookies_t m_cookies {};
	value_list_t m_chunk_attributes {};
};

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT>::basic_client_request(url_t url) :
	m_impl(new impl(std::move(url)))
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT>::basic_client_request() :
	basic_client_request(url_t())
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT>::~basic_client_request()
{
	delete m_impl;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT>::basic_client_request(basic_client_request &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl({});
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::operator=(basic_client_request &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl({});
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_url(url_t url)
{
	m_impl->m_url = std::move(url);
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_method(http::method method)
{
	method_check(method);
	m_impl->m_method = method;
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_header(string_view_t key, value_t value) noexcept
{
	m_impl->m_headers[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_cookie(string_view_t key, value_t value) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_chunk_attribute(value_t attribute)
{
	set_header(basic_header<CharT>::transfer_encoding, string_pool::chunked);
	m_impl->m_chunk_attributes.emplace_back(std::move(attribute));
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::set_chunk_attributes(value_list_t attributes)
{
	set_header(basic_header<CharT>::transfer_encoding, string_pool::chunked);
	for(auto &value : attributes)
		m_impl->m_chunk_attributes.emplace_back(std::move(value));
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
http::method basic_client_request<Stream,CharT>::method() const noexcept
{
	return m_impl->m_method;
}

template <concept_stream_requires Stream, concept_char_type CharT>
const basic_headers<CharT> &basic_client_request<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <concept_stream_requires Stream, concept_char_type CharT>
const basic_cookie_values<CharT> &basic_client_request<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <concept_stream_requires Stream, concept_char_type CharT>
const basic_value_list<CharT> &basic_client_request<Stream,CharT>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <concept_stream_requires Stream, concept_char_type CharT>
const basic_url<CharT> &basic_client_request<Stream,CharT>::url() const noexcept
{
	return m_impl->m_url;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_url<CharT> &basic_client_request<Stream,CharT>::url() noexcept
{
	return m_impl->m_url;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::unset_header(string_view_t key)
{
	m_impl->m_headers.erase({key.data(), key.size()});
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_cookies.erase({key.data(), key.size()});
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::unset_chunk_attribute(const value_t &attributes)
{
	m_impl->m_chunk_attributes.erase(attributes);
	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream,CharT>::reset()
{
	m_impl->m_path = string_pool::root;
	m_impl->m_chunk_attributes.clear();
	m_impl->m_cookies.clear();
	m_impl->m_headers = {
		{ header_t::content_type, string_pool::text_plain }
	};
	return *this;
}




template <concept_stream_requires Stream, concept_char_type CharT>
typename basic_client_request<Stream,CharT>::endpoint_t basic_client_request<Stream,CharT>::remote_endpoint() const
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
typename basic_client_request<Stream,CharT>::endpoint_t basic_client_request<Stream,CharT>::local_endpoint() const
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
const typename basic_client_request<Stream,CharT>::executor_t &basic_client_request<Stream,CharT>::get_executor() noexcept
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
basic_client_request<Stream,CharT> &basic_client_request<Stream, CharT>::cancel() noexcept
{

	return *this;
}

template <concept_stream_requires Stream, concept_char_type CharT>
const typename basic_client_request<Stream,CharT>::next_layer_t &basic_client_request<Stream,CharT>::next_layer() const noexcept
{

}

template <concept_stream_requires Stream, concept_char_type CharT>
typename basic_client_request<Stream,CharT>::next_layer_t &basic_client_request<Stream,CharT>::next_layer() noexcept
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H