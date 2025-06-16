
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

#include "parser.h"

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN parser<model::client>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(size_t init_buf_size) :
		m_parser(init_buf_size)
	{
		m_parser
		.on_parse_begin([this](std::string_view line_buf, error_code &error)
		{
			// TODO ... ...
			return version_enum::v11;
		})
		.on_parse_cookie([this](std::string_view line_buf, error_code &error)
		{
			// TODO ... ...
		});
	}

public:
	base_parser m_parser;
	status_enum m_status = status::ok;
	std::string m_description = status::description<status::ok>();
	cookies_t m_cookies {};
};

parser<model::client>::parser(size_t init_buf_size) :
	m_impl(new impl(init_buf_size))
{

}

parser<model::client>::~parser()
{
	delete m_impl;
}

parser<model::client>::parser(parser &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(0xFFFF);
}

parser<model::client> &parser<model::client>::operator=(parser &&other) noexcept
{
	if( this == &other )
        return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl(0xFFFF);
	return *this;
}

bool parser<model::client>::append(const const_buffer &buf, error_code &error)
{

	return false;
}

bool parser<model::client>::append(const const_buffer &buf)
{

	return false;
}

bool parser<model::client>::operator<<(const const_buffer &buf)
{

	return false;
}

std::string_view parser<model::client>::version() const noexcept
{

	return "";
}

status_enum parser<model::client>::status() const noexcept
{

	return status_enum::service_unavailable;
}

const headers &parser<model::client>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

const cookies &parser<model::client>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

bool parser<model::client>::keep_alive() const noexcept
{

	return false;
}

bool parser<model::client>::support_gzip() const noexcept
{

	return false;
}

bool parser<model::client>::can_read_from_device() const noexcept
{

	return false;
}

std::string parser<model::client>::take_partial_body(size_t size)
{

	return "";
}

std::string parser<model::client>::take_body()
{

	return "";
}

bool parser<model::client>::is_finished() const noexcept
{

	return false;
}

bool parser<model::client>::is_eof() const noexcept
{

	return false;
}

parser<model::client> &parser<model::client>::reset()
{

	return *this;
}

} //namespace libgs::http::protocol