
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H

namespace libgs::http::protocol
{

template <typename Derived>
const_parameters<Derived>::const_parameters(const parameters_t *parameters) :
	m_parameters(parameters)
{

}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it == parameters().end() )
		return nullopt;
	return it->second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const core_concepts::text_p<char> auto &key, const value_t &value) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it != parameters().end() )
		return it->second == value;
	return false;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	return it != parameters().end();
}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(size_t index) const
{
	if( not contains_parameter(index) )
		runtime_error::loc_throw("index out of range.");
	return parameters()[index].second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter(size_t index) const noexcept
{
	return index >= parameters().size();
}

template <typename Derived>
const const_parameters<Derived>::parameters_t&
const_parameters<Derived>::parameters() const noexcept
{
	return *m_parameters;
}

template <typename Derived>
const_headers<Derived>::const_headers(const headers_t *headers) :
	m_headers(headers)
{

}

template <typename Derived>
optional<typename const_headers<Derived>::value_t>
const_headers<Derived>::header(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	if( it == headers().end() )
		return nullopt;
	return it->second;
}

template <typename Derived>
bool const_headers<Derived>::contains_header
(const core_concepts::text_p<char> auto &key, const value_t &value) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	if( it != headers().end() )
		return it->second == value;
	return false;
}

template <typename Derived>
bool const_headers<Derived>::contains_header
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	return it != headers().end();
}

template <typename Derived>
const const_headers<Derived>::headers_t&
const_headers<Derived>::headers() const noexcept
{
	return *m_headers;
}

template <typename Cookie, typename Derived>
const_cookies<Cookie,Derived>::const_cookies(const cookies_t *cookies) :
	m_cookies(cookies)
{

}

template <typename Cookie, typename Derived>
optional<typename const_cookies<Cookie,Derived>::cookie_t>
const_cookies<Cookie,Derived>::cookie(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = cookies().find(strtls::to_string(key));
	if( it == cookies().end() )
		return nullopt;
	return it->second;
}

template <typename Cookie, typename Derived>
bool const_cookies<Cookie,Derived>::contains_cookie
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = cookies().contains(strtls::to_string(key));
	return it != cookies().end();
}

template <typename Cookie, typename Derived>
const const_cookies<Cookie,Derived>::cookies_t&
const_cookies<Cookie,Derived>::cookies() const noexcept
{
	return *m_cookies;
}

template <typename Derived>
const_chunk_attributes<Derived>::const_chunk_attributes(const values_t *chunk_attributes) :
	m_chunk_attributes(chunk_attributes)
{

}

template <typename Derived>
bool const_chunk_attributes<Derived>::contains_chunk_attribute(const value_t &attr) const noexcept
{
	return chunk_attributes().find(attr) != chunk_attributes().end();
}

template <typename Derived>
const const_chunk_attributes<Derived>::values_t&
const_chunk_attributes<Derived>::chunk_attributes() const noexcept
{
	return *m_chunk_attributes;
}

template <typename Derived>
mutable_parameters<Derived>::base_t::derived_t &mutable_parameters<Derived>::set_parameter
(core_concepts::text_p<char> auto &&key, typename base_t::value_t value) noexcept
{
	parameters()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_parameters<Derived>::base_t::derived_t &mutable_parameters<Derived>::unset_parameter
(const core_concepts::text_p<char> auto &key) noexcept
{
	parameters().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_parameters<Derived>::base_t::parameters_t&
mutable_parameters<Derived>::parameters() noexcept
{
	return remove_const(*this->m_parameters);
}

template <typename Derived>
mutable_headers<Derived>::base_t::derived_t &mutable_headers<Derived>::set_header
(core_concepts::text_p<char> auto &&key, typename base_t::value_t value) noexcept
{
	headers()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_headers<Derived>::base_t::derived_t &mutable_headers<Derived>::unset_header
(const core_concepts::text_p<char> auto &key) noexcept
{
	headers().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_headers<Derived>::base_t::headers_t &mutable_headers<Derived>::headers() noexcept
{
	return remove_const(*this->m_headers);
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::derived_t &mutable_cookies<Cookie,Derived>::set_cookie
(core_concepts::text_p<char> auto &&key, typename base_t::cookie_t value) noexcept
{
	cookies()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::derived_t &mutable_cookies<Cookie,Derived>::unset_cookie
(const core_concepts::text_p<char> auto &key) noexcept
{
	cookies().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::cookies_t&
mutable_cookies<Cookie,Derived>::cookies() noexcept
{
	return remove_const(*this->m_cookies);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::derived_t&
mutable_chunk_attributes<Derived>::set_chunk_attribute(typename base_t::value_t attr) noexcept
{
	if( auto [it, inserted] = chunk_attributes().emplace(std::move(attr)); not inserted )
	{
		chunk_attributes().erase(it);
		chunk_attributes().emplace(std::move(attr));
	}
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::derived_t&
mutable_chunk_attributes<Derived>::unset_chunk_attribute(const typename base_t::value_t &attr) noexcept
{
	chunk_attributes().erase(attr);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::values_t&
mutable_chunk_attributes<Derived>::chunk_attributes() noexcept
{
	return remove_const(*this->m_chunk_attributes);
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H