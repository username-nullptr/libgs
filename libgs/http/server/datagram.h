
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

#ifndef LIBGS_HTTP_SERVER_DATAGRAM_H
#define LIBGS_HTTP_SERVER_DATAGRAM_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
struct basic_server_datagram : basic_datagram<CharT>
{
	LIBGS_DISABLE_COPY(basic_server_datagram)
	basic_server_datagram() = default;

	using str_type = std::basic_string<CharT>;
	using headers_type = basic_headers<CharT>;
	using cookies_type = basic_cookies<CharT>;
	using parameters_type = basic_parameters<CharT>;

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

	basic_server_datagram(basic_server_datagram&&) noexcept = default;
	basic_server_datagram &operator=(basic_server_datagram&&) noexcept = default;
};

using server_datagram = basic_server_datagram<char>;
using server_wdatagram = basic_server_datagram<wchar_t>;

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DATAGRAM_H
