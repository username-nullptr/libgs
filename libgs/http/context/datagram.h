
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

#ifndef LIBGS_HTTP_CONTEXT_DETAIL_DATAGRAM_H
#define LIBGS_HTTP_CONTEXT_DETAIL_DATAGRAM_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_datagram
{
	LIBGS_DISABLE_COPY(basic_datagram)

public:
	class impl;
	using str_type = std::basic_string<CharT>;
	using headers_type = basic_headers<CharT>;
	using parameters_type = basic_parameters<CharT>;
	using cookies_type = basic_cookies<CharT>;

public:
	basic_datagram();
	~basic_datagram();

	basic_datagram(basic_datagram &&other) noexcept;
	basic_datagram &operator=(basic_datagram &&other) noexcept;

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_type version() const noexcept;

	[[nodiscard]] const str_type &path() const noexcept;
	[[nodiscard]] const str_type &parameter_string() const noexcept;

	[[nodiscard]] const headers_type &headers() const noexcept;
	[[nodiscard]] const cookies_type &cookies() const noexcept;

	[[nodiscard]] std::string &partial_body() const noexcept;
	[[nodiscard]] bool can_read_body() const noexcept;

public:
	[[nodiscard]] bool is_websocket_handshake() const noexcept;
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;

public:
	void set_request_header(std::string_view request_header);
	void set_response_header(std::string_view response_header);
	void add_header(std::string_view header_line);
	void set_partial_body(std::string &partial_body, bool finished = true);

private:
	impl *m_impl;
};

using datagram = basic_datagram<char>;
using wdatagram = basic_datagram<wchar_t>;

} //namespace libgs::http
#include <libgs/http/context/detail/datagram.h>

#include <libgs/core/string_list.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_datagram<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;
	void set_method(std::string_view method)
	{
		if( method == "GET" )
			m_method = method::GET;
		else if( method == "PUT" )
			m_method = method::PUT;
		else if( method == "POST" )
			m_method = method::POST;
		else if( method == "HEAD" )
			m_method = method::HEAD;
		else if( method == "DELETE" )
			m_method = method::DELETE;
		else if( method == "OPTIONS" )
			m_method = method::OPTIONS;
		else if( method == "CONNECT" )
			m_method = method::CONNECT;
		else if( method == "TRACH" )
			m_method = method::TRACH;
		else
			throw runtime_error("libgs::http::datagram: Invalid http method: '{}'.", method);
	}

	void set_version(std::string_view version)
	{
		if constexpr( is_char_v<CharT> )
			m_version = std::string(version.data(), version.size());
		else
			m_version = mbstowcs(version);
	}

	void set_path_and_parameter(std::string_view path_line)
	{
		auto list = string_list::from_string(path_line, '?');
		if( list.size() < 2 )
			throw runtime_error("libgs::http::datagram::set_path_and_parameter: Invalid path line.");
	}

public:
	http::method m_method;
	str_type m_version;

	str_type m_path;
	str_type m_parameter_string;

	headers_type m_headers;
	cookies_type m_cookies;

	parameters_type m_parameters;
	std::string m_partial_body;

	bool m_keep_alive = false;
	bool m_support_gzip = false;
	bool m_websocket = false;
	bool m_finished = false;
};

template <concept_char_type CharT>
basic_datagram<CharT>::basic_datagram() :
	m_impl(new impl())
{

}

template <concept_char_type CharT>
basic_datagram<CharT>::~basic_datagram()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_datagram<CharT>::basic_datagram(basic_datagram &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_char_type CharT>
basic_datagram<CharT> &basic_datagram<CharT>::operator=(basic_datagram &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_char_type CharT>
http::method basic_datagram<CharT>::method() const noexcept
{
	return m_impl->m_method;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_datagram<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <concept_char_type CharT>
const std::basic_string<CharT> &basic_datagram<CharT>::path() const noexcept
{
	return m_impl->m_path;
}

template <concept_char_type CharT>
const std::basic_string<CharT> &basic_datagram<CharT>::parameter_string() const noexcept
{
	return m_impl->m_parameter_string;
}

template <concept_char_type CharT>
const basic_headers<CharT> &basic_datagram<CharT>::headers() const noexcept
{
	return m_impl->m_headers;
}

template <concept_char_type CharT>
const basic_cookies<CharT> &basic_datagram<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <concept_char_type CharT>
std::string &basic_datagram<CharT>::partial_body() const noexcept
{
	return m_impl->m_partial_body;
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::can_read_body() const noexcept
{
	return not m_impl->m_finished;
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::is_websocket_handshake() const noexcept
{
	return m_impl->m_websocket;
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::keep_alive() const noexcept
{
	return m_impl->m_keep_alive;
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::support_gzip() const noexcept
{
	return m_impl->m_support_gzip;
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_request_header(std::string_view request_header)
{
	auto str_list = string_list::from_string(request_header, ' ');
	if( str_list.size() != 3 )
		throw runtime_error("libgs::http::datagram: Invalid http first line (client request): '{}'.", request_header);

	m_impl->set_method(str_list[0]);
	m_impl->set_method(str_list[0]);
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_response_header(std::string_view response_header)
{
	auto str_list = string_list::from_string(response_header, ' ');
	if( str_list.size() != 3 )
		throw runtime_error("libgs::http::datagram: Invalid http first line (server response): '{}'.", response_header);

}

template <concept_char_type CharT>
void basic_datagram<CharT>::add_header(std::string_view header_line)
{

}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_partial_body(std::string &partial_body, bool finished)
{
	m_impl->m_finished = finished;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CONTEXT_DETAIL_DATAGRAM_H
