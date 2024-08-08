
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
	using string_pool = detail::string_pool<CharT>;

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using value_list_t = basic_value_list<CharT>;

	using header_t = basic_header<CharT>;
	using headers_t = basic_headers<CharT>;

	using cookie_t = basic_cookie<CharT>;
	using cookies_t = basic_cookies<CharT>;

public:
	explicit basic_response_helper(string_view_t version, const headers_t &request_headers = {});
	explicit basic_response_helper(const headers_t &request_headers = {}); // default V1.1
	~basic_response_helper();

	basic_response_helper(basic_response_helper &&other) noexcept ;
	basic_response_helper &operator=(basic_response_helper &&other) noexcept;

public:
	basic_response_helper &set_status(uint32_t status);
	basic_response_helper &set_status(http::status status);

	basic_response_helper &set_header(string_view_t key, value_t value) noexcept;
	basic_response_helper &set_cookie(string_view_t key, cookie_t cookie) noexcept;

	basic_response_helper &set_redirect(string_view_t url, redirect type = redirect::moved_permanently);

	basic_response_helper &set_chunk_attribute(value_t attribute);
	basic_response_helper &set_chunk_attributes(value_list_t attributes);

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

	[[nodiscard]] const value_list_t &chunk_attributes() const noexcept;

public:
	basic_response_helper &unset_header(string_view_t key);
	basic_response_helper &unset_cookie(string_view_t key);
	basic_response_helper &unset_chunk_attribute(const value_t &attributes);
	basic_response_helper &reset();

private:
	class impl;
	impl *m_impl;
};

using response_helper = basic_response_helper<char>;
using wresponse_helper = basic_response_helper<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/response_helper.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H