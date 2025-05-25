
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

class LIBGS_HTTP_API request_parser final
{
	LIBGS_DISABLE_COPY(request_parser)

public:
	using value_t = libgs::value;
	using path_args_t = std::vector<std::pair<std::string,value_t>>;

public:
	explicit request_parser(size_t init_buf_size = 0xFFFF);
	~request_parser();

	request_parser(request_parser &&other) noexcept;
	request_parser &operator=(request_parser &&other) noexcept;

public:
	bool append(const const_buffer &buf, error_code &error);
	bool append(const const_buffer &buf);

	request_parser &operator<<(const const_buffer &buf);
	[[nodiscard]] int32_t path_match(std::string_view rule);

public:
	[[nodiscard]] method_enum method() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;
	[[nodiscard]] version_enum version() const noexcept;

public:
	[[nodiscard]] const http::parameters &parameters() const noexcept;
	[[nodiscard]] const path_args_t &path_args() const noexcept;
	[[nodiscard]] const http::headers &headers() const noexcept;
	[[nodiscard]] const cookie_values &cookies() const noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool can_read_from_device() const noexcept;

public:
	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;
	request_parser &reset();

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_REQUEST_PARSER_H