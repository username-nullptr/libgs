
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

#ifndef LIBGS_HTTP_PARSER_BASE_H
#define LIBGS_HTTP_PARSER_BASE_H

#include <libgs/http/types.h>

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

class LIBGS_HTTP_API parser_base final
{
	LIBGS_DISABLE_COPY(parser_base)

public:
	using parse_begin_handler = std::function <
		version_t(std::string_view line_buf, error_code &error)
	>;
	using parse_cookie_handler = std::function <
		void(std::string_view line_buf, error_code &error)
	>;

public:
	explicit parser_base(size_t init_buf_size = 0xFFFF);
	~parser_base();

	parser_base(parser_base &&other) noexcept;
	parser_base &operator=(parser_base &&other) noexcept;

public:
	parser_base &on_parse_begin(parse_begin_handler func);
	parser_base &on_parse_cookie(parse_cookie_handler func);
	[[nodiscard]] static error_code make_error_code(parse_errno errc);

	bool append(const const_buffer &buf, error_code &error);
	bool append(const const_buffer &buf);

	parser_base &operator<<(const const_buffer &buf);
	parser_base &reset();

public:
	[[nodiscard]] version_t version() const noexcept;
	[[nodiscard]] const http::headers &headers() const noexcept;

	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();

public:
	[[nodiscard]] bool can_read_from_device() const noexcept;
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

public:
	parser_base &unset_parse_begin();
	parser_base &unset_parse_cookie();

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http


#endif //LIBGS_HTTP_PARSER_BASE_H
