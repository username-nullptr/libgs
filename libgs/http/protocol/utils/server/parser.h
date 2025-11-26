
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H

#include <libgs/http/protocol/utils/core/types.h>

namespace libgs::http::protocol
{

template <>
class LIBGS_HTTP_API parser<model::server> final
{
	LIBGS_DISABLE_COPY(parser)

public:
	using stage_t = protocol::stage;

	using value_t = libgs::value;
	using path_args_t = parameter_map;

	using parameters_t = protocol::parameters;
	using headers_t = protocol::headers;

public:
	explicit parser(size_t init_buf_size = 0xFFFF);
	~parser();

	parser(parser &&other) noexcept;
	parser &operator=(parser &&other) noexcept;

public:
	sys_expected<bool> append(const const_buffer &buf);
	parser &operator<<(const const_buffer &buf);
	[[nodiscard]] int32_t path_match(std::string_view rule);

public:
	[[nodiscard]] method_enum method() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;
	[[nodiscard]] version_enum version() const noexcept;

public:
	[[nodiscard]] optional<value_t> parameter(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] optional<value_t> parameter(size_t index) const;
	[[nodiscard]] const parameters_t &parameters() const noexcept;

	[[nodiscard]] optional<value_t> header(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;

	[[nodiscard]] optional<value_t> cookie(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const cookie_values &cookies() const noexcept;

	[[nodiscard]] optional<value_t> path_arg (const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] optional<value_t> path_arg(size_t index) const;
	[[nodiscard]] const path_args_t &path_args() const noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;

	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();

	[[nodiscard]] stage_t stage() const noexcept;
	parser &reset();

private:
	class impl;
	impl *m_impl;
};

using server_parser = parser<model::server>;

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/server/detail/parser.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H