
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REPLY_PARSER_H
#define LIBGS_HTTP_CLIENT_DETAIL_REPLY_PARSER_H

#include <libgs/http/parser_base.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_reply_parser<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using parser_t = basic_parser_base<char_t>;

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
	status_t m_status = status::ok;
	string_t m_description = status_description<status::ok,char_t>();
	cookies_t m_cookies {};
};

template <core_concepts::char_type CharT>
basic_reply_parser<CharT>::basic_reply_parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

template <core_concepts::char_type CharT>
basic_reply_parser<CharT>::~basic_reply_parser()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_reply_parser<CharT>::basic_reply_parser(basic_reply_parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

template <core_concepts::char_type CharT>
basic_reply_parser<CharT> &basic_reply_parser<CharT>::operator=(basic_reply_parser &&other) noexcept
{
	if( this == &other )
        return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::append(core_concepts::string_type auto &&buf, error_code &error)
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::append(core_concepts::string_type auto &&buf)
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::operator<<(core_concepts::string_type auto &&buf)
{

}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_reply_parser<CharT>::version() const noexcept
{

}

template <core_concepts::char_type CharT>
status_t basic_reply_parser<CharT>::status() const noexcept
{

}

template <core_concepts::char_type CharT>
const basic_value<CharT> &basic_reply_parser<CharT>::header(string_view_t key) const
{

}

template <core_concepts::char_type CharT>
const basic_cookie<CharT> &basic_reply_parser<CharT>::cookie(string_view_t key) const
{

}

template <core_concepts::char_type CharT>
basic_value<CharT> basic_reply_parser<CharT>::header_or(string_view_t key, value_t def_value) const noexcept
{

}

template <core_concepts::char_type CharT>
basic_cookie<CharT> basic_reply_parser<CharT>::cookie_or(string_view_t key, value_t def_value) const noexcept
{

}

template <core_concepts::char_type CharT>
const basic_headers<CharT> &basic_reply_parser<CharT>::headers() const noexcept
{

}

template <core_concepts::char_type CharT>
const basic_cookies<CharT> &basic_reply_parser<CharT>::cookies() const noexcept
{

}

template <core_concepts::char_type CharT>
const basic_value_list<CharT> &basic_reply_parser<CharT>::chunk_attributes() const noexcept
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::keep_alive() const noexcept
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::support_gzip() const noexcept
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::can_read_from_device() const noexcept
{

}

template <core_concepts::char_type CharT>
std::string basic_reply_parser<CharT>::take_partial_body(size_t size)
{

}

template <core_concepts::char_type CharT>
std::string basic_reply_parser<CharT>::take_body()
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::is_finished() const noexcept
{

}

template <core_concepts::char_type CharT>
bool basic_reply_parser<CharT>::is_eof() const noexcept
{

}

template <core_concepts::char_type CharT>
basic_reply_parser<CharT> &basic_reply_parser<CharT>::reset()
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REPLY_PARSER_H
