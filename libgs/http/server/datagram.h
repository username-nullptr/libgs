
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
struct LIBGS_HTTP_TAPI basic_server_datagram : basic_datagram<CharT>
{
	LIBGS_DISABLE_COPY(basic_server_datagram)
	basic_server_datagram() = default;

	using base_type = basic_datagram<CharT>;
	using str_type = base_type::str_type;
	using headers_type = base_type::headers_type;

	using cookies_type = std::map<std::basic_string<CharT>, basic_value<CharT>, less_case_insensitive>;
	using parameters_type = basic_parameters<CharT>;

	http::method method = http::method::GET;
	str_type path;
	parameters_type parameters;
	cookies_type cookies;

	basic_server_datagram(basic_server_datagram&&) noexcept = default;
	basic_server_datagram &operator=(basic_server_datagram&&) noexcept = default;
};

using server_datagram = basic_server_datagram<char>;
using server_wdatagram = basic_server_datagram<wchar_t>;

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DATAGRAM_H
