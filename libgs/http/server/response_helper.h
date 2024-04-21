
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
	using value_list_type = basic_value_list<CharT>;

	using header_type = basic_header<CharT>;
	using headers_type = basic_headers<CharT>;

	using cookie_type = basic_cookie<CharT>;
	using cookies_type = basic_cookies<CharT>;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	explicit basic_response_helper(str_view_type version, const headers_type &request_headers);
	~basic_response_helper();

	basic_response_helper(basic_response_helper &&other) noexcept ;
	basic_response_helper &operator=(basic_response_helper &&other) noexcept;

public:
	this_type &set_status(uint32_t status);
	this_type &set_status(http::status status);

	this_type &set_header(str_view_type key, value_type value) noexcept;
	this_type &set_cookie(str_view_type key, cookie_type cookie) noexcept;

	this_type &set_redirect(str_view_type url, redirect_type type = redirect_type::moved_permanently);

	this_type &set_chunk_attribute(value_type attribute);
	this_type &set_chunk_attributes(value_list_type attributes);

public:
	using write_callback = std::function<awaitable<size_t>(std::string_view,error_code&)>;
	this_type &on_write(write_callback writer, std::function<size_t()> get_write_buffer_size = {});

	[[nodiscard]] awaitable<size_t> write(opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(buffer<std::string_view> body, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(std::string_view file_name, opt_token<ranges,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> chunk_end(opt_token<const headers_type&, error_code&> tk = {});

public:
	[[nodiscard]] str_view_type version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const headers_type &headers() const noexcept;
	[[nodiscard]] const cookies_type &cookies() const noexcept;

	[[nodiscard]] bool headers_writed() const noexcept;
	[[nodiscard]] bool chunk_end_writed() const noexcept;

public:
	this_type &unset_header(str_view_type key);
	this_type &unset_cookie(str_view_type key);
	this_type &unset_chunk_attribute(value_type key);
	this_type &reset();

private:
	class impl;
	impl *m_impl;
};

using response_helper = basic_response_helper<char>;
using wresponse_helper = basic_response_helper<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/response_helper.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H