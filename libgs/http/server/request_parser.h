
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_PARSER_H
#define LIBGS_HTTP_SERVER_REQUEST_PARSER_H

#include <libgs/http/server/request_datagram.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser
{
	LIBGS_DISABLE_COPY(basic_request_parser)

public:
	explicit basic_request_parser(size_t init_buf_size = 0xFFFF);
	~basic_request_parser();

	basic_request_parser(basic_request_parser &&other) noexcept;
	basic_request_parser &operator=(basic_request_parser &&other) noexcept;

public:
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);

public:
	using datagram_type = basic_request_datagram<CharT>;
	datagram_type get_result();
//	datagram_view_type datagram();

private:
	class impl;
	impl *m_impl;
};

using request_parser = basic_request_parser<char>;
using wrequest_parser = basic_request_parser<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/request_parser.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_PARSER_H
