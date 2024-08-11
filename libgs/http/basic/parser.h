
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

#ifndef LIBGS_HTTP_BASIC_PARSER_H
#define LIBGS_HTTP_BASIC_PARSER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_parser
{
	LIBGS_DISABLE_COPY(basic_parser)

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using value_list_t = basic_value_list<CharT>;

	using cookies_t = basic_cookie_values<CharT>;
	using header_t = basic_header<CharT>;
	using headers_t = basic_headers<CharT>;

public:
	basic_parser();
	~basic_parser();

	basic_parser(basic_parser &&other) noexcept;
	basic_parser &operator=(basic_parser &&other) noexcept;

public:

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/basic/detail/parser.h>


#endif //LIBGS_HTTP_BASIC_PARSER_H
