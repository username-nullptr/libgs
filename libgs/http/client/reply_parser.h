
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

#ifndef LIBGS_HTTP_CLIENT_REPLY_PARSER_H
#define LIBGS_HTTP_CLIENT_REPLY_PARSER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_reply_parser
{
	LIBGS_DISABLE_COPY(basic_reply_parser)

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using value_list_t = basic_value_list<CharT>;

	using cookie_t = basic_cookie<CharT>;
	using cookies_t = basic_cookies<CharT>;

	using header_t = basic_header<CharT>;
	using headers_t = basic_headers<CharT>;

public:
	explicit basic_reply_parser(size_t init_buf_size = 0xFFFF);
	~basic_reply_parser();

	basic_reply_parser(basic_reply_parser &&other) noexcept;
	basic_reply_parser &operator=(basic_reply_parser &&other) noexcept;

public:
	bool append(std::string_view buf, error_code &error);
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const value_t &header(string_view_t key) const;
	[[nodiscard]] const cookie_t &cookie(string_view_t key) const;

	[[nodiscard]] value_t header_or(string_view_t key, value_t def_value = {}) const noexcept;
	[[nodiscard]] cookie_t cookie_or(string_view_t key, value_t def_value = {}) const noexcept;

public:
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] const value_list_t &chunk_attributes() const noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool can_read_from_device() const noexcept;

public:
	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;
	basic_reply_parser &reset();

private:
	class impl;
	impl *m_impl;
};

using reply_parser = basic_reply_parser<char>;
using wreply_parser = basic_reply_parser<wchar_t>;

} //namespace libgs::http
#include <libgs/http/client/detail/reply_parser.h>


#endif //LIBGS_HTTP_CLIENT_REPLY_PARSER_H
