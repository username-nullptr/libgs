
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
	class data;
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using headers_type = basic_headers<CharT>;
	using parameters_type = basic_parameters<CharT>;
	using cookies_type = basic_cookies<CharT>;

public:
	basic_datagram();
	~basic_datagram();

	basic_datagram(basic_datagram &&other) noexcept;
	basic_datagram &operator=(basic_datagram &&other) noexcept;

public:
	[[nodiscard]] http::method method() const;
	[[nodiscard]] str_type version() const;

	[[nodiscard]] const str_type &path() const;
	[[nodiscard]] const str_type &parameter_string() const;

	[[nodiscard]] const headers_type &headers() const;
	[[nodiscard]] const cookies_type &cookies() const;

	[[nodiscard]] std::string &partial_body() const;
	[[nodiscard]] bool can_read_body() const;

public:
	[[nodiscard]] bool is_websocket_handshake() const;
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool is_valid() const;

public:
	void set_method(str_view_type method);
	void set_version(str_type version);
	void set_path_and_parameter(str_view_type path_line);
	void add_header_or_cookie(str_view_type header_line);
	void set_partial_body(std::string &partial_body);

private:
	data *m_data;
};

using datagram = basic_datagram<char>;
using wdatagram = basic_datagram<wchar_t>;

} //namespace libgs::http
#include <libgs/http/context/detail/datagram.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_datagram<CharT>::data
{
	LIBGS_DISABLE_COPY_MOVE(data)

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
};

template <concept_char_type CharT>
basic_datagram<CharT>::basic_datagram() :
	m_data(new data())
{

}

template <concept_char_type CharT>
basic_datagram<CharT>::~basic_datagram()
{
	delete m_data;
}

template <concept_char_type CharT>
basic_datagram<CharT>::basic_datagram(basic_datagram &&other) noexcept :
	m_data(other.m_data)
{
	other.m_data = new data();
}

template <concept_char_type CharT>
basic_datagram<CharT> &basic_datagram<CharT>::operator=(basic_datagram &&other) noexcept
{
	m_data = other.m_data;
	other.m_data = new data();
	return *this;
}

template <concept_char_type CharT>
http::method basic_datagram<CharT>::method() const
{
	return m_data->m_method;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_datagram<CharT>::version() const
{
}

template <concept_char_type CharT>
const std::basic_string<CharT> &basic_datagram<CharT>::path() const
{
}

template <concept_char_type CharT>
const std::basic_string<CharT> &basic_datagram<CharT>::parameter_string() const
{
}

template <concept_char_type CharT>
const basic_headers<CharT> &basic_datagram<CharT>::headers() const
{
}

template <concept_char_type CharT>
const basic_cookies<CharT> &basic_datagram<CharT>::cookies() const
{
}

template <concept_char_type CharT>
std::string &basic_datagram<CharT>::partial_body() const
{
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::can_read_body() const
{
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::is_websocket_handshake() const
{
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::keep_alive() const
{
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::support_gzip() const
{
}

template <concept_char_type CharT>
bool basic_datagram<CharT>::is_valid() const
{
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_method(str_view_type method)
{
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_version(str_type version)
{
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_path_and_parameter(str_view_type path_line)
{
}

template <concept_char_type CharT>
void basic_datagram<CharT>::add_header_or_cookie(str_view_type header_line)
{
}

template <concept_char_type CharT>
void basic_datagram<CharT>::set_partial_body(std::string &partial_body)
{
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CONTEXT_DETAIL_DATAGRAM_H
