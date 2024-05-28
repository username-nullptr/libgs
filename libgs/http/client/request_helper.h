
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_HELPER_H
#define LIBGS_HTTP_CLIENT_REQUEST_HELPER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_helper
{
	LIBGS_DISABLE_COPY(basic_request_helper)
	using this_type = basic_request_helper;

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using value_list_t = basic_value_list<CharT>;

public:
	explicit basic_request_helper(string_view_t version);
	~basic_request_helper();

	basic_request_helper(basic_request_helper &&other) noexcept ;
	basic_request_helper &operator=(basic_request_helper &&other) noexcept;

public:
	this_type &set_header(string_view_t key, value_t value) noexcept;
};

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_REQUEST_HELPER_H