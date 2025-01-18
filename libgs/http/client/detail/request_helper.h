
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H

namespace libgs::http
{

template <core_concepts::char_type CharT, version_t Version>
class LIBGS_HTTP_TAPI basic_request_helper<CharT,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using string_list_t = basic_string_list<char_t>;
	using string_pool = detail::string_pool<char_t>;
	using header_t = typename request_arg_t::header_t;

public:
	explicit impl(request_arg_t &request) :
		m_request_arg(&request)
	{
		if constexpr( Version < 1.1 )
		{
			auto it = m_request_arg->headers().find(header::transfer_encoding);
			if( it != m_request_arg->headers().end() and it->second == string_pool::chunked )
			{
				throw runtime_error (
					"libgs::http::request_helper: Only HTTP/1.1 supports 'Transfer-Coding: chunked'."
				);
			}
		}
	}

public:
	[[nodiscard]] std::string header_data(size_t body_size)
	{
		auto &headers = m_request_arg->headers();
		auto it = headers.find(header_t::content_length);

		if( it == headers.end() )
		{
			if constexpr( Version > 1.0 )
			{
				it = headers.find(header_t::transfer_encoding);
				if( it == headers.end() or str_to_lower(it->second.to_string()) != string_pool::chunked )
					m_request_arg->set_header(header_t::content_length, body_size);
			}
			else
				m_request_arg->set_header(header_t::content_length, body_size);
		}
		auto buf = to_method_string(m_request_arg->method()) + " ";
		auto url = xxtombs(m_request_arg->path());

		if( not m_request_arg->parameters().empty() )
		{
			url += "?";
			for(auto &[key,value] : m_request_arg->parameters())
				url += xxtombs(key) + "=" + xxtombs(value) + "&";
		}
		buf += to_percent_encoding(url) + " HTTP/" + version_string<Version>() + "\r\n";
		for(auto &[key,value] : headers)
			buf += xxtombs(key) + ": " + xxtombs(value) + "\r\n";

		if( not m_request_arg->cookies().empty() )
		{
			buf += "Cookie: ";
			for(auto &[key,value] : m_request_arg->cookies())
				buf += xxtombs(key) + "=" + xxtombs(value) + ";";
			buf += "\r\n";
		}
		return buf + "\r\n";
	}

	[[nodiscard]] std::string body_data(const const_buffer &buffer)
	{
		// TODO ... ...
		LIBGS_UNUSED(buffer);
		return {};
	}

	[[nodiscard]] std::string chunk_end_data(const headers_t &headers)
	{
		// TODO ... ...
		LIBGS_UNUSED(headers);
		return {};
	}

public:
	request_arg_t *m_request_arg = nullptr;
};

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(request_arg_t &request) :
	m_impl(new impl(request))
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(string_view_t version, request_arg_t &request) :
	m_impl(new impl(request, version))
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::~basic_request_helper()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(const basic_request_helper &other) noexcept :
	m_impl(new impl(*other.m_impl->m_request_arg))
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>&
basic_request_helper<CharT,Version>::operator=(const basic_request_helper &other) noexcept
{
	m_impl->m_request_arg = other.m_impl->m_request_arg;
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::header_data(size_t body_size)
{
	return m_impl->header_data(body_size);
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::body_data(const const_buffer &buffer)
{
	return m_impl->body_data(buffer);
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::chunk_end_data(const headers_t &headers)
{
	return m_impl->chunk_end_data(headers);
}

template <core_concepts::char_type CharT, version_t Version>
consteval version_t basic_request_helper<CharT,Version>::version() const noexcept
{
	return version_v;
}

template <core_concepts::char_type CharT, version_t Version>
const typename basic_request_helper<CharT,Version>::request_arg_t&
basic_request_helper<CharT,Version>::request_arg() const noexcept
{
	return m_impl->m_request_arg;
}

template <core_concepts::char_type CharT, version_t Version>
typename basic_request_helper<CharT,Version>::request_arg_t&
basic_request_helper<CharT,Version>::request_arg() noexcept
{
	return m_impl->m_request_arg;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
