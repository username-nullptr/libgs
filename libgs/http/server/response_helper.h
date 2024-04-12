
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

#ifndef LIBGS_HTTP_SERVER_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_RESPONSE_HELPER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_response_helper
{
	LIBGS_DISABLE_COPY(basic_response_helper)

public:
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;
	using headers_type = basic_headers<CharT>;

public:
	explicit basic_response_helper(str_view_type version, const headers_type &headers);
	~basic_response_helper() = default;

public:

private:
	class impl;
	impl *m_impl;
};

using response_helper = basic_response_helper<char>;
using wresponse_helper = basic_response_helper<wchar_t>;

} //namespace libgs::http

namespace libgs::http
{

template <concept_char_type CharT>
class basic_response_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
};

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(str_view_type version, const headers_type &headers)
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H