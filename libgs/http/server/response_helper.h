
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
	using this_type = basic_response_helper;

public:
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using value_type = basic_value<CharT>;
	using headers_type = basic_headers<CharT>;

	using cookie_type = basic_cookie<CharT>;
	using cookies_type = basic_cookies<CharT>;

public:
	explicit basic_response_helper(str_view_type version, const headers_type &headers);
	~basic_response_helper();

public:
	this_type &set_status(uint32_t status);
	this_type &set_status(http::status status);

	this_type &set_header(str_view_type key, value_type value);
	this_type &set_cookie(std::string key, cookie_type cookie);

	this_type &redirect(str_view_type url, redirect_type type = redirect_type::moved_permanently);
	this_type &send_file(std::string_view file_name, http::ranges ranges = {});

public:

public:
	[[nodiscard]] str_view_type version() const;
	[[nodiscard]] http::status status() const;

	[[nodiscard]] const headers_type &headers() const;
	[[nodiscard]] const cookies_type &cookies() const;

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
	impl(str_view_type version, const headers_type &headers) :
		m_version(version), m_request_headers(headers) {}

public:
	str_view_type m_version;
	const headers_type &m_request_headers;

	http::status m_status;
	headers_type m_response_headers;

	str_type m_redirect_url;
	std::string m_file_name;
};

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(str_view_type version, const headers_type &headers) :
	m_impl(new impl(version, headers))
{

}

template <concept_char_type CharT>
basic_response_helper<CharT>::~basic_response_helper()
{
	delete m_impl;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H