
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H

#include <libgs/http/protocol/utils/core/parser.h>

namespace libgs::http::protocol
{

template <>
class LIBGS_HTTP_API parser<model::server> final
{
	LIBGS_DISABLE_COPY(parser)

public:
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
	bool append(const const_buffer &buf, error_code &error);
	bool append(const const_buffer &buf);

	parser &operator<<(const const_buffer &buf);
	[[nodiscard]] int32_t path_match(std::string_view rule);

public:
	[[nodiscard]] method_enum method() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;
	[[nodiscard]] version_enum version() const noexcept;

public:
	template <typename T = value_t>
	[[nodiscard]] decltype(auto) parameter (
		const core_concepts::text_p<char> auto &key
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) parameter(size_t index)
		const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) parameter_or (
		const core_concepts::text_p<char> auto &key, T &&def_value = {}
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) parameter_or(size_t index, T &&def_value = {})
		const requires core_concepts::value_get<T,char>;

	[[nodiscard]] const parameters_t &parameters() const noexcept;

public:
	template <typename T = value>
	[[nodiscard]] decltype(auto) header (
		const core_concepts::text_p<char> auto &key
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value>
	[[nodiscard]] decltype(auto) header_or (
		const core_concepts::text_p<char> auto &key, T &&def_value = {}
	) const requires core_concepts::value_get<T,char>;

	[[nodiscard]] const headers_t &headers() const noexcept;

public:
	template <typename T = value_t>
	[[nodiscard]] decltype(auto) cookie (
		const core_concepts::text_p<char> auto &key
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) cookie_or (
		const core_concepts::text_p<char> auto &key, T &&def_value = {}
	) const requires core_concepts::value_get<T,char>;

	[[nodiscard]] const cookie_values &cookies() const noexcept;

public:
	template <typename T = value_t>
	[[nodiscard]] decltype(auto) path_arg (
		const core_concepts::text_p<char> auto &key
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) path_arg(size_t index)
		const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) path_arg_or (
		const core_concepts::text_p<char> auto &key, T &&def_value = {}
	) const requires core_concepts::value_get<T,char>;

	template <typename T = value_t>
	[[nodiscard]] decltype(auto) path_arg_or(size_t index, T &&def_value = {})
		const requires core_concepts::value_get<T,char>;

	[[nodiscard]] const path_args_t &path_args() const noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool can_read_from_device() const noexcept;

public:
	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;
	parser &reset();

private:
	class impl;
	impl *m_impl;
};

using server_parser = parser<model::server>;

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/server/detail/parser.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_SERVER_PARSER_H