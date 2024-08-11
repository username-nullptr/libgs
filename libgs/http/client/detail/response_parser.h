
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
	using parser_t = basic_parser<CharT>;

public:
	explicit impl(size_t init_buf_size) :
		m_parser(init_buf_size)
	{
		m_parser
		.on_parse_begin([this](std::string_view line_buf, error_code &error)
		{

		})
		.on_parse_cookie([this](std::string_view line_buf, error_code &error)
		{

		});
	}

public:
	parser_t m_parser;
	http::status m_status = http::status::ok;
	string_t m_description = basic_status_description_v<CharT,http::status::ok>;
	cookies_t m_cookies {};
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

template <concept_char_type CharT>
bool basic_response_parser<CharT>::append(std::string_view buf, error_code &error)
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::append(std::string_view buf)
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::operator<<(std::string_view buf)
{

}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_response_parser<CharT>::version() const noexcept
{

}

template <concept_char_type CharT>
http::status basic_response_parser<CharT>::status() const noexcept
{

}

template <concept_char_type CharT>
const basic_value<CharT> &basic_response_parser<CharT>::header(string_view_t key) const
{

}

template <concept_char_type CharT>
const basic_cookie<CharT> &basic_response_parser<CharT>::cookie(string_view_t key) const
{

}

template <concept_char_type CharT>
basic_value<CharT> basic_response_parser<CharT>::header_or(string_view_t key, value_t def_value) const noexcept
{

}

template <concept_char_type CharT>
basic_cookie<CharT> basic_response_parser<CharT>::cookie_or(string_view_t key, value_t def_value) const noexcept
{

}

template <concept_char_type CharT>
const basic_headers<CharT> &basic_response_parser<CharT>::headers() const noexcept
{

}

template <concept_char_type CharT>
const basic_cookies<CharT> &basic_response_parser<CharT>::cookies() const noexcept
{

}

template <concept_char_type CharT>
const basic_value_list<CharT> &basic_response_parser<CharT>::chunk_attributes() const noexcept
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::keep_alive() const noexcept
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::support_gzip() const noexcept
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::can_read_from_device() const noexcept
{

}

template <concept_char_type CharT>
std::string basic_response_parser<CharT>::take_partial_body(size_t size)
{

}

template <concept_char_type CharT>
std::string basic_response_parser<CharT>::take_body()
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::is_finished() const noexcept
{

}

template <concept_char_type CharT>
bool basic_response_parser<CharT>::is_eof() const noexcept
{

}

template <concept_char_type CharT>
basic_response_parser<CharT> &basic_response_parser<CharT>::reset()
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_RESPONSE_PARSER_H
