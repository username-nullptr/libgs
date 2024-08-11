
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_RESPONSE_PARSER_H
#define LIBGS_HTTP_CLIENT_DETAIL_RESPONSE_PARSER_H

#include <libgs/http/basic/parser.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_response_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(size_t init_buf_size) {
		m_src_buf.reserve(init_buf_size);
	}

public:
	enum class state
	{
		waiting_request,      // HTTP/1.1 200 OK\r\n
		reading_headers,      // Key: Value\r\n
		reading_length,       // Fixed length (Content-Length: 9\r\n).
		chunked_wait_size,    // 9\r\n
		chunked_wait_content, // body\r\n
		chunked_wait_headers, // Key: Value\r\n
		finished
	}
	m_state = state::waiting_request;
	size_t m_state_context = 0;
	std::string m_src_buf;

	http::method m_method = http::method::GET;
	http::status m_status;

	string_t m_version;
	string_t m_description;
	string_t m_path;

	headers_t m_headers;
	cookies_t m_cookies;
	std::string m_partial_body;
};

template <concept_char_type CharT>
basic_response_parser<CharT>::basic_response_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <concept_char_type CharT>
basic_response_parser<CharT>::~basic_response_parser()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_response_parser<CharT>::basic_response_parser(basic_response_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <concept_char_type CharT>
basic_response_parser<CharT> &basic_response_parser<CharT>::operator=(basic_response_parser &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}


} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_RESPONSE_PARSER_H
