
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

#ifndef LIBGS_HTTP_DETAIL_HELPER_BASE_H
#define LIBGS_HTTP_DETAIL_HELPER_BASE_H

#include <set>

namespace libgs::http
{

template <version_enum Version>
class LIBGS_HTTP_TAPI basic_helper_base<Version>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;
	headers_t m_headers {
		{ header::content_type, "text/plain; charset=utf-8" }
	};
	std::set<value_t> m_chunk_attributes {};
	size_t m_content_length = 0;
	state_t m_state {};
};

template <version_enum Version>
basic_helper_base<Version>::basic_helper_base() :
	m_impl(new impl())
{

}

template <version_enum Version>
basic_helper_base<Version>::~basic_helper_base()
{
	delete m_impl;
}

template <version_enum Version>
basic_helper_base<Version>::basic_helper_base(basic_helper_base &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <version_enum Version>
basic_helper_base<Version> &basic_helper_base<Version>::operator=(basic_helper_base &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <version_enum Version>
basic_helper_base<Version> &basic_helper_base<Version>::set_header
(core_concepts::text_p<char> auto &&key, value_t value) noexcept
{
	m_impl->m_headers[strtls::to_string(std::forward<decltype(key)>(key))]
		= std::forward<value_t>(value);
	return *this;
}

template <version_enum Version>
basic_helper_base<Version> &basic_helper_base<Version>::unset_header
(const core_concepts::text_p<char> auto &key) noexcept
{
	m_impl->m_headers.erase(strtls::to_string(key));
	return *this;
}

template <version_enum Version>
const headers &basic_helper_base<Version>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <version_enum Version>
headers &basic_helper_base<Version>::headers() noexcept
{
	return m_impl->m_headers;
}

template <version_enum Version>
basic_helper_base<Version>&
basic_helper_base<Version>::set_chunk_attribute(value_t attr) noexcept
{
	auto [it, inserted] = m_impl->m_chunk_attributes.emplace(std::move(attr));
	if( not inserted )
	{
		m_impl->m_chunk_attributes.erase(it);
		m_impl->m_chunk_attributes.emplace(std::move(attr));
	}
	return *this;
}

template <version_enum Version>
basic_helper_base<Version>&
basic_helper_base<Version>::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_chunk_attributes.erase(attr);
	return *this;
}

template <version_enum Version>
const std::set<value> &basic_helper_base<Version>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <version_enum Version>
basic_helper_base<Version> &basic_helper_base<Version>::reset()
{
	headers().clear();
	chunk_attributes().clear();
	m_impl->m_state = state_t::header;
	return *this;
}

template <version_enum Version>
std::string basic_helper_base<Version>::header_data(size_t body_size)
{
	if( state() != state_t::header )
		return {};

	auto &headers = this->headers();
	auto it = headers.find(header::content_length);

	if( it == headers.end() )
	{
		if constexpr( version_v > version::v10 )
		{
			constexpr auto chunked_text = "chunked";
			it = headers.find(header::transfer_encoding);

			if( it != headers.end() and strtls::to_lower(*it->second) == chunked_text )
				m_impl->m_state = state_t::chunk;
			else
			{
				m_impl->m_content_length = body_size;
				m_impl->m_state = state_t::content_length;
				m_impl->m_headers[header::content_length] = m_impl->m_content_length;
			}
		}
		else
		{
			m_impl->m_content_length = body_size;
			m_impl->m_state = state_t::content_length;
			m_impl->m_headers[header::content_length] = m_impl->m_content_length;
		}
	}
	else
	{
		m_impl->m_content_length = it->second.template get<size_t>();
		m_impl->m_state = state_t::content_length;
	}
	std::string buf;
	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf;
}

template <version_enum Version>
std::string basic_helper_base<Version>::body_data(const const_buffer &buffer)
{
	if( m_impl->m_state == state_t::header or m_impl->m_state == state_t::finish )
		return {};

	else if( m_impl->m_state == state_t::content_length )
	{
		size_t size = 0;
		if( m_impl->m_content_length > buffer.size() )
		{
			size = buffer.size();
			m_impl->m_content_length -= size;
		}
		else
		{
			size = m_impl->m_content_length;
			m_impl->m_content_length = 0;
			m_impl->m_state = state_t::finish;
		}
		return {reinterpret_cast<const char*>(buffer.data()), size};
	}
	std::string sum;
	if( m_impl->m_chunk_attributes.empty() )
		sum += std::format("{:X}\r\n", buffer.size());
	else
	{
		std::string attributes;
		for(auto &attr : m_impl->m_chunk_attributes)
			attributes += *attr + ";";

		m_impl->m_chunk_attributes.clear();
		attributes.pop_back();
		sum += std::format("{:X}; {}\r\n", buffer.size(), attributes);
	}
	return sum + std::string(static_cast<const char*>(buffer.data()), buffer.size()) + "\r\n";
}

template <version_enum Version>
std::string basic_helper_base<Version>::chunk_end_data(const headers_t &headers)
{
	if( m_impl->m_state != state_t::chunk )
		return {};

	m_impl->m_state = state_t::finish;
	std::string buf = "0\r\n";

	for(auto &[key,value] : headers)
		buf += key + ": " + *value + "\r\n";
	return buf + "\r\n";
}

template <version_enum Version>
std::set<value> &basic_helper_base<Version>::chunk_attributes() noexcept
{
	return m_impl->m_chunk_attributes;
}

template <version_enum Version>
consteval version_enum basic_helper_base<Version>::version() const noexcept
{
	return version_v;
}

template <version_enum Version>
typename basic_helper_base<Version>::state_t
basic_helper_base<Version>::state() const noexcept
{
	return m_impl->m_state;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_HELPER_BASE_H