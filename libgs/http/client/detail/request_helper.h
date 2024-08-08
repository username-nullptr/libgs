
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

template <concept_char_type CharT>
class basic_request_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using string_list_t = basic_string_list<CharT>;
	using string_pool = detail::string_pool<CharT>;
	using header_t = typename request_t::header_t;

public:
	explicit impl(request_t &request, string_view_t version = string_pool::v_1_1) :
		m_version(version.data(), version.size()), m_request(&request)
	{
		if( stof(version()) < 1.1 )
		{
			auto it = m_request->headers().find(header::transfer_encoding);
			if( it != m_request->headers().end() and it->second == string_pool::chunked )
				throw runtime_error("libgs::http::request_helper: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");
		}
	}

	impl(impl &&other) noexcept :
		m_version(std::move(other)), m_request(other.m_request)
	{
		other.m_version = string_pool::v_1_1;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_version = std::move(other);
		m_request = other.m_request;
		other.m_version = string_pool::v_1_1;
		return *this;
	}

public:
	std::string header_data(size_t body_size)
	{
		auto &headers = m_request->headers();
		auto it = headers.find(header_t::content_length);

		if( it == headers.end() )
		{
			if( stoi32(m_version) > 1.0 )
			{
				it = headers.find(header_t::transfer_encoding);
				if( it == headers.end() or str_to_lower(it->second.to_string()) != string_pool::chunked )
					m_request->set_header(header_t::content_length, body_size);
			}
			else
				m_request->set_header(header_t::content_length, body_size);
		}
		auto buf = to_method_string(m_request->method()) + " ";
		auto url = xxtombs<CharT>(m_request->path());

		if( not m_request->parameters().empty() )
		{
			url += "?";
			for(auto &[key,value] : m_request->parameters())
				url += xxtombs<CharT>(key) + "=" + xxtombs<CharT>(value) + "&";
		}
		buf += to_percent_encoding(url) + " HTTP/" + m_version + "\r\n";
		for(auto &[key,value] : headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value) + "\r\n";

		if( not m_request->cookies().empty() )
		{
			buf += "Cookie: ";
			for(auto &[key,value] : m_request->cookies())
				buf += xxtombs<CharT>(key) + "=" + xxtombs<CharT>(value) + ";";
			buf += "\r\n";
		}
		return buf + "\r\n";
	}

public:
	string_t m_version = string_pool::v_1_1;
	request_t *m_request = nullptr;
};

template <concept_char_type CharT>
basic_request_helper<CharT>::basic_request_helper(string_view_t version, request_t &request) :
	m_impl(new impl(request, version))
{

}

template <concept_char_type CharT>
basic_request_helper<CharT>::basic_request_helper(request_t &request) :
	m_impl(new impl(request))
{

}

template <concept_char_type CharT>
basic_request_helper<CharT>::~basic_request_helper()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_request_helper<CharT>::basic_request_helper(basic_request_helper &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::operator=(basic_request_helper &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_char_type CharT>
std::string basic_request_helper<CharT>::header_data(size_t body_size)
{
	return m_impl->header_data(body_size);
}

template <concept_char_type CharT>
std::string basic_request_helper<CharT>::body_data(const const_buffer &buffer)
{
	return m_impl->body_data(buffer);
}

template <concept_char_type CharT>
std::string basic_request_helper<CharT>::chunk_end_data(const headers_t &headers)
{
	return m_impl->chunk_end_data(headers);
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_request_helper<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <concept_char_type CharT>
const basic_client_request<CharT> &basic_request_helper<CharT>::request() const noexcept
{
	return m_impl->m_request;
}

template <concept_char_type CharT>
basic_client_request<CharT> &basic_request_helper<CharT>::request() noexcept
{
	return m_impl->m_request;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
