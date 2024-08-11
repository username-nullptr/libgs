
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

#ifndef LIBGS_HTTP_BASIC_PARSER_H
#define LIBGS_HTTP_BASIC_PARSER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

#define LIBGS_HTTP_PARSER_ERRNO \
X_MACRO( RLTL , 10000 , "Request line too long."      ) \
X_MACRO( HLTL , 10001 , "Header line too long."       ) \
X_MACRO( IRL  , 10002 , "Invalid request line."       ) \
X_MACRO( IHM  , 10003 , "Invalid http method."        ) \
X_MACRO( IHP  , 10004 , "Invalid http path."          ) \
X_MACRO( IHL  , 10005 , "Invalid header line."        ) \
X_MACRO( IDE  , 10006 , "The inserted data is empty." ) \
X_MACRO( SFE  , 10007 , "Size format error."          ) \
X_MACRO( RE   , 10008 , "This request is ended."      )

enum class parse_errno
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
};

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_parser
{
	LIBGS_DISABLE_COPY(basic_parser)

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using header_t = basic_header<CharT>;
	using headers_t = basic_headers<CharT>;

	using parse_begin_handler = std::function<string_t(std::string_view line_buf, error_code &error)>;
	using parse_cookie_handler = std::function<void(std::string_view line_buf, error_code &error)>;

public:
	explicit basic_parser(size_t init_buf_size = 0xFFFF);
	~basic_parser();

	basic_parser(basic_parser &&other) noexcept;
	basic_parser &operator=(basic_parser &&other) noexcept;

public:
	basic_parser &on_parse_begin(parse_begin_handler func);
	basic_parser &on_parse_cookie(parse_cookie_handler func);
	static error_code make_error_code(parse_errno errc);

	bool append(std::string_view buf, error_code &error);
	bool append(std::string_view buf);
	bool operator<<(std::string_view buf);
	basic_parser &reset();

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;

	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();

public:
	[[nodiscard]] bool can_read_from_device() const noexcept;
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

public:
	basic_parser &unset_parse_begin();
	basic_parser &unset_parse_cookie();

private:
	class impl;
	impl *m_impl;
};

using parser = basic_parser<char>;
using wparser = basic_parser<wchar_t>;

} //namespace libgs::http
#include <libgs/http/basic/detail/parser.h>


#endif //LIBGS_HTTP_BASIC_PARSER_H
