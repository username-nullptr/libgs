
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_PARSER_H
#define LIBGS_HTTP_SERVER_REQUEST_PARSER_H

#include <libgs/http/types.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser final
{
	LIBGS_DISABLE_COPY(basic_request_parser)

public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = basic_value<char_t>;
	using header_t = basic_header<char_t>;
	using headers_t = basic_headers<char_t>;

	using parameters_t = basic_parameters<char_t>;
	using cookies_t = basic_cookie_values<char_t>;
	using path_args_t = std::vector<std::pair<string_t,value_t>>;

public:
	explicit basic_request_parser(size_t init_buf_size = 0xFFFF);
	~basic_request_parser();

	basic_request_parser(basic_request_parser &&other) noexcept;
	basic_request_parser &operator=(basic_request_parser &&other) noexcept;

public:
	bool append(const const_buffer &buf, error_code &error);
	bool append(const const_buffer &buf);

	basic_request_parser &operator<<(const const_buffer &buf);
	[[nodiscard]] int32_t path_match(string_view_t rule);

public:
	[[nodiscard]] method_t method() const noexcept;
	[[nodiscard]] string_view_t path() const noexcept;
	[[nodiscard]] string_view_t version() const noexcept;

public:
	[[nodiscard]] const parameters_t &parameters() const noexcept;
	[[nodiscard]] const path_args_t &path_args() const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool can_read_from_device() const noexcept;

public:
	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;
	basic_request_parser &reset();

private:
	class impl;
	impl *m_impl;
};

using request_parser = basic_request_parser<char>;
using wrequest_parser = basic_request_parser<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/request_parser.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_PARSER_H