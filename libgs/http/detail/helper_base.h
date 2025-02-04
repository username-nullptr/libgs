
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

template <core_concepts::char_type CharT, version_t Version>
class LIBGS_HTTP_TAPI basic_helper_base<CharT,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;
	headers_t m_headers;
	std::set<value_t> m_chunk_attributes {};
	size_t m_content_length = 0;
	state_t m_state {};
};

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>::basic_helper_base() :
	m_impl(new impl())
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>::~basic_helper_base()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>::basic_helper_base(basic_helper_base &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::operator=(basic_helper_base &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
template <typename...Args>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::set_header(Args&&...args) noexcept requires
	concepts::set_key_attr_params<char_t,Args...>
{
	set_map(m_impl->m_headers, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::set_header(pair_init_t headers) noexcept
{
	set_map(m_impl->m_headers, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
template <typename...Args>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::set_chunk_attribute(Args&&...args) noexcept requires
	concepts::set_attr_params<char_t,Args...>
{
	set_set(m_impl->m_chunk_attributes, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::set_chunk_attribute(attr_init_t attributes) noexcept
{
	set_set(m_impl->m_chunk_attributes, std::move(attributes));
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
const typename basic_helper_base<CharT,Version>::headers_t&
basic_helper_base<CharT,Version>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <core_concepts::char_type CharT, version_t Version>
const typename basic_helper_base<CharT,Version>::value_set_t&
basic_helper_base<CharT,Version>::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

template <core_concepts::char_type CharT, version_t Version>
consteval version_t basic_helper_base<CharT,Version>::version() const noexcept
{
	return version_v;
}

template <core_concepts::char_type CharT, version_t Version>
typename basic_helper_base<CharT,Version>::state_t
basic_helper_base<CharT,Version>::state() const noexcept
{
	return m_impl->m_state;
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_helper_base<CharT,Version>::header_data(size_t body_size)
{
	if( state() != state_t::header )
		return {};

	auto &headers = this->headers();
	auto it = headers.find(header_t::content_length);

	if( it == headers.end() )
	{
		if constexpr( version_v > 1.0 )
		{
			constexpr auto chunked_text = detail::string_pool<char_t>::chunked;
			it = headers.find(header_t::transfer_encoding);

			if( it != headers.end() and str_to_lower(it->second.to_string()) == chunked_text )
				m_impl->m_state = state_t::chunk;
			else
			{
				m_impl->m_content_length = body_size;
				m_impl->m_state = state_t::content_length;
				m_impl->m_headers[header_t::content_length] = m_impl->m_content_length;
			}
		}
		else
		{
			m_impl->m_content_length = body_size;
			m_impl->m_state = state_t::content_length;
			m_impl->m_headers[header_t::content_length] = m_impl->m_content_length;
		}
	}
	else
	{
		m_impl->m_content_length = it->second.template get<size_t>();
		m_impl->m_state = state_t::content_length;
	}
	std::string buf;
	for(auto &[key,value] : headers)
		buf += xxtombs(key) + ": " + xxtombs(value.to_string()) + "\r\n";
	return buf;
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_helper_base<CharT,Version>::body_data(const const_buffer &buffer)
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
			attributes += xxtombs(attr.to_string()) + ";";

		m_impl->m_chunk_attributes.clear();
		attributes.pop_back();
		sum += std::format("{:X}; {}\r\n", buffer.size(), attributes);
	}
	return sum + std::string(static_cast<const char*>(buffer.data()), buffer.size()) + "\r\n";
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_helper_base<CharT,Version>::chunk_end_data(const headers_t &headers)
{
	if( m_impl->m_state != state_t::chunk )
		return {};

	auto it = m_impl->m_headers.find(header_t::content_length);
	if( it != m_impl->m_headers.end() )
		throw runtime_error("libgs::http::helper_base::chunk_end: 'Content-Length' has been set.");

	constexpr auto chunked_text = detail::string_pool<char_t>::chunked;
	it = m_impl->m_headers.find(header_t::transfer_encoding);

	if( it == m_impl->m_headers.end() or str_to_lower(it->second) != chunked_text )
		throw runtime_error("libgs::http::helper_base::chunk_end: 'Transfer-Coding: chunked' not set.");

	std::string buf = "0\r\n";
	for(auto &[key,value] : headers)
		buf += xxtombs(key) + ": " + xxtombs(value.to_string()) + "\r\n";
	return buf + "\r\n";
}

template <core_concepts::char_type CharT, version_t Version>
template <typename...Args>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::unset_header(Args&&...args)
	requires concepts::unset_pair_params<char_t,Args...>
{
	unset_map(m_impl->m_headers, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::unset_header(key_init_t headers) noexcept
{
	unset_map(m_impl->m_headers, std::move(headers));
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::clear_headers() noexcept
{
	m_impl->m_headers.clear();
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
template <typename...Args>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::unset_chunk_attribute(Args&&...args)
	requires concepts::unset_attr_params<char_t,Args...>
{
	unset_set(m_impl->m_chunk_attributes, std::forward<Args>(args)...);
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version>&
basic_helper_base<CharT,Version>::unset_chunk_attribute(attr_init_t attributes) noexcept
{
	unset_set(m_impl->m_chunk_attributes, std::move(attributes));
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::clear_chunk_attributes() noexcept
{
	m_impl->m_chunk_attributes.clear();
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_helper_base<CharT,Version> &basic_helper_base<CharT,Version>::reset()
{
	clear_headers();
	clear_chunk_attributes();
	m_impl->m_state = state_t::header;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_HELPER_BASE_H