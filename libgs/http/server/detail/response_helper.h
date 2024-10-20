
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

#include <libgs/core/algorithm/mime_type.h>

namespace libgs::http
{

namespace detail
{

template <typename T>
struct _response_helper_static_string;

#define LIBGS_HTTP_DETAIL_STATIC_STRING(_type, ...) \
	static constexpr const _type *colon                 = __VA_ARGS__##": "                             ; \
	static constexpr const _type *line_break            = __VA_ARGS__##"\r\n"                           ; \
	static constexpr const _type *bytes                 = __VA_ARGS__##"bytes"                          ; \
	static constexpr const _type *bytes_start           = __VA_ARGS__##"bytes="                         ; \
	static constexpr const _type *range_format          = __VA_ARGS__##"{}-{},"                         ; \
	static constexpr const _type *content_range_format  = __VA_ARGS__##"{}-{}/{}"                       ; \
	static constexpr const _type *content_type_boundary = __VA_ARGS__##"multipart/byteranges; boundary="; \

template <>
struct _response_helper_static_string<char> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(char);
};

template <>
struct _response_helper_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STATIC_STRING

} //namespace detail

template <core_concepts::char_type CharT>
class basic_response_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	struct string_pool : detail::string_pool<CharT>, detail::_response_helper_static_string<CharT> {};

public:
	impl() = default;
	impl(const headers_t &request_headers, string_view_t version = string_pool::v_1_1) :
		m_version(version), m_request_headers(&request_headers) {}

public:
	[[nodiscard]] std::string header_data(size_t body_size)
	{
		auto it = m_response_headers.find(header_t::content_length);
		if( it == m_response_headers.end() )
		{
			if( stoi32(m_version) > 1.0 )
			{
				it = m_response_headers.find(header_t::transfer_encoding);
				if( it == m_response_headers.end() or str_to_lower(it->second.to_string()) != string_pool::chunked )
					set_header(header_t::content_length, body_size);
			}
			else
				set_header(header_t::content_length, body_size);
		}
		std::string buf;
		buf.reserve(4096);

		buf = std::format("HTTP/{} {} {}\r\n", xxtombs<CharT>(m_version), m_status, status_description(m_status));
		m_response_headers.erase(string_pool::set_cookie);

		for(auto &[key,value] : m_response_headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value.to_string()) + "\r\n";

		for(auto &[ckey,cookie] : m_cookies)
		{
			buf += "set-cookie: " + xxtombs<CharT>(ckey) + "=" + xxtombs<CharT>(cookie.value().to_string()) + ";";
			for(auto &[akey,attr] : cookie.attributes())
				buf += xxtombs<CharT>(akey) + "=" + xxtombs<CharT>(attr.to_string()) + ";";

			buf.pop_back();
			buf += "\r\n";
		}
		return buf + "\r\n";
	}

	[[nodiscard]] std::string body_data(const void *buf, size_t size)
	{
		if( not is_chunked() )
			return {static_cast<const char*>(buf), size};

		std::string sum;
		if( m_chunk_attributes.empty() )
			sum += std::format("{:X}\r\n", size);
		else
		{
			std::string attributes;
			for(auto &attr : m_chunk_attributes)
				attributes += xxtombs<CharT>(attr.to_string()) + ";";

			m_chunk_attributes.clear();
			attributes.pop_back();
			sum += std::format("{:X}; {}\r\n", size, attributes);
		}
		return sum + std::string(static_cast<const char*>(buf), size) + "\r\n";
	}

	[[nodiscard]] std::string chunk_end_data(const headers_t &headers)
	{
		auto it = m_response_headers.find(header_t::content_length);
		if( it != m_request_headers->end() )
			throw runtime_error("libgs::http::response_helper::chunk_end: 'Content-Length' has been set.");

		it = m_response_headers.find(header_t::transfer_encoding);
		if( it == m_response_headers.end() or str_to_lower(it->second) != string_pool::chunked )
			throw runtime_error("libgs::http::response_helper::chunk_end: 'Transfer-Coding: chunked' not set.");

		std::string buf = "0\r\n";
		for(auto &[key,value] : headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value.to_string()) + "\r\n";
		return buf + "\r\n";
	}

	void set_header(string_view_t key, value_t value) noexcept {
		m_response_headers[str_to_lower(key)] = std::move(value);
	}

private:
	[[nodiscard]] bool is_chunked() const
	{
		if( stoi32(m_version) < 1.1 )
			return false;

		auto it = m_request_headers->find(header_t::transfer_encoding);
		return it != m_request_headers->end() and str_to_lower(it->second.to_string()) == string_pool::chunked;
	}

public:
	string_view_t m_version = string_pool::v_1_1;
	const headers_t *m_request_headers = nullptr;

	status_t m_status = status::ok;
	headers_t m_response_headers {
		{ header_t::content_type, string_pool::text_plain }
	};
	cookies_t m_cookies {};
	value_list_t m_chunk_attributes {};
	string_t m_redirect_url {};
};

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::basic_response_helper(string_view_t version, const headers_t &request_headers) :
	m_impl(new impl(request_headers, version))
{

}

template <core_concepts::char_type CharT>
basic_response_helper<CharT>::basic_response_helper(const headers_t &request_headers) :
	m_impl(new impl(request_headers))
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
basic_response_helper<CharT> &basic_response_helper<CharT>::set_header(string_view_t key, value_t value) noexcept
{
	m_impl->set_header(key, std::move(value));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_cookie(string_view_t key, cookie_t cookie) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(cookie);
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_redirect(string_view_t url, redirect type)
{
	switch(type)
	{
#define X_MACRO(e,v) case redirect::e : set_status(v); break;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		default: throw runtime_error("libgs::http::response_helper::redirect: Invalid redirect type: '{}'.", type);
	}
	return set_header(header_t::location, url);
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attribute(value_t attribute)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	set_header(basic_header<CharT>::transfer_encoding, string_pool::chunked);
	m_impl->m_chunk_attributes.emplace_back(std::move(attribute));
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attributes(value_list_t attributes)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	set_header(basic_header<CharT>::transfer_encoding, string_pool::chunked);
	for(auto &value : attributes)
		m_impl->m_chunk_attributes.emplace_back(std::move(value));
	return *this;
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::header_data(size_t body_size)
{
	return m_impl->header_data(body_size);
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::body_data(const const_buffer &buffer)
{
	return m_impl->body_data(buffer.data(), buffer.size());
}

template <core_concepts::char_type CharT>
std::string basic_response_helper<CharT>::chunk_end_data(const headers_t &headers)
{
	return m_impl->chunk_end_data(headers);
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_response_helper<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <core_concepts::char_type CharT>
status_t basic_response_helper<CharT>::status() const noexcept
{
	return m_impl->m_status;
}

template <core_concepts::char_type CharT>
const typename basic_response_helper<CharT>::headers_t &basic_response_helper<CharT>::headers() const noexcept
{
	return m_impl->m_response_headers;
}

template <core_concepts::char_type CharT>
const typename basic_response_helper<CharT>::cookies_t &basic_response_helper<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <core_concepts::char_type CharT>
const basic_value_list<CharT> &basic_response_helper<CharT>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_header(string_view_t key)
{
	m_impl->m_response_headers.erase({key.data(), key.size()});
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_cookies.erase({key.data(), key.size()});
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_chunk_attribute(const value_t &attributes)
{
	m_impl->m_chunk_attributes.erase(attributes);
	return *this;
}

template <core_concepts::char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::reset()
{
	m_impl->m_version = string_pool::v_1_1;
	m_impl->m_status = status::ok;

	m_impl->m_response_headers = {
		{ header_t::content_type, string_pool::text_plain }
	};
	m_impl->m_cookies.clear();
	m_impl->m_chunk_attributes.clear();
	m_impl->m_redirect_url.clear();

	m_impl->m_chunk_end_writed = false;
	m_impl->m_headers_writed = false;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
