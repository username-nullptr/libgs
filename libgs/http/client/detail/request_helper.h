
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

public:
	impl() = default;
	explicit impl(request_arg_t &request) :
		m_req_arg(std::move(request)) {}

	helper_t m_helper;
	request_arg_t m_req_arg;
	headers_t m_auto_headers;
};

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(request_arg_t request) :
	m_impl(new impl(request))
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(string_view_t version, request_arg_t request) :
	m_impl(new impl(request, version))
{

}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::~basic_request_helper()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>::basic_request_helper(basic_request_helper &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>&
basic_request_helper<CharT,Version>::operator=(basic_request_helper &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version>&
basic_request_helper<CharT,Version>::set_arg(request_arg_t arg)
{
	m_impl->m_req_arg = std::move(arg);
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
const typename basic_request_helper<CharT,Version>::request_arg_t&
basic_request_helper<CharT,Version>::arg() const noexcept
{
	return m_impl->m_req_arg;
}

template <core_concepts::char_type CharT, version_t Version>
typename basic_request_helper<CharT,Version>::request_arg_t&
basic_request_helper<CharT,Version>::arg() noexcept
{
	return m_impl->m_req_arg;
}

template <core_concepts::char_type CharT, version_t Version>
template <method_t Method>
std::string basic_request_helper<CharT,Version>::header_data(size_t body_size)
{
	return header_data(Method, body_size);
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::header_data(method_t method, size_t body_size)
{
	if( state() != state_t::header )
		return {};

	auto &arg = this->arg();
	auto &url = arg.url();

	auto buf = std::string(method_string(method)) + " ";
	{
		string_t path(xxtombs(url.path()));
		if( not url.parameters().empty() )
		{
			path += "?";
			string_t params;

			for(auto &[key,value] : url.parameters())
				params += xxtombs(key) + "=" + xxtombs(value.to_string()) + "&";

			params.pop_back();
			path += to_percent_encoding(params);
		}
		buf += path + " HTTP/" + version_string<version_v>() + "\r\n";
	}
	for(auto &pair : arg.headers())
		m_impl->m_helper.set_header(pair);

	buf += m_impl->m_helper.header_data(body_size);
	if( not arg.cookies().empty() )
	{
		buf += "Cookie: ";
		for(auto &[key,value] : arg.cookies())
			buf += xxtombs(key) + "=" + xxtombs(value.to_string()) + ";";
		buf += "\r\n";
	}
	return buf + "\r\n";
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::body_data(const const_buffer &buffer)
{
	return m_impl->m_helper.body_data(buffer);
}

template <core_concepts::char_type CharT, version_t Version>
std::string basic_request_helper<CharT,Version>::chunk_end_data(const headers_t &headers)
{
	return m_impl->m_helper.chunk_end_data(headers);
}

template <core_concepts::char_type CharT, version_t Version>
typename basic_request_helper<CharT,Version>::state_t
basic_request_helper<CharT,Version>::state() const noexcept
{
	return m_impl->m_helper.state();
}

template <core_concepts::char_type CharT, version_t Version>
basic_request_helper<CharT,Version> &basic_request_helper<CharT,Version>::reset() noexcept
{
	m_impl->m_auto_headers.clear();
	m_impl->m_helper.reset();
	return *this;
}

template <core_concepts::char_type CharT, version_t Version>
consteval version_t basic_request_helper<CharT,Version>::version() const noexcept
{
	return version_v;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
