
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

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_request_parser
{
	LIBGS_DISABLE_COPY(basic_request_parser)

public:
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using cookies_type = basic_cookie_values<CharT>;
	using header_type = basic_header<CharT>;
	using headers_type = basic_headers<CharT>;
	using parameters_type = basic_parameters<CharT>;

public:
	explicit basic_request_parser(size_t init_buf_size = 0xFFFF);
	~basic_request_parser();

	basic_request_parser(basic_request_parser &&other) noexcept;
	basic_request_parser &operator=(basic_request_parser &&other) noexcept;

public:
	bool append(std::string_view buf, error_code &error);
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_view_type path() const noexcept;
	[[nodiscard]] str_view_type version() const noexcept;

public:
	[[nodiscard]] const parameters_type &parameters() const noexcept;
	[[nodiscard]] const headers_type &headers() const noexcept;
	[[nodiscard]] const cookies_type &cookies() const noexcept;

public:
	[[nodiscard]] bool is_websocket_handshake() const;
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool can_read_body() const;

public:
	[[nodiscard]] std::string take_partial_body(size_t size = 0);
	[[nodiscard]] bool is_finished() const noexcept;
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
