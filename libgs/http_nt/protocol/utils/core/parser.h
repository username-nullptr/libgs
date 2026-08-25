
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_PARSER_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_PARSER_H

#include <libgs/http_nt/protocol/utils/core/container_helper.h>
#include <libgs/http_nt/protocol/utils/core/parser_types.h>

namespace libgs::http_nt
{

template <>
class LIBGS_HTTP_NT_API parser<protocol_model::base> final :
	public const_headers<parser<protocol_model::base>>
{
	LIBGS_DISABLE_COPY(parser)

public:
	using stage_t = http_nt::stage;
	using chunk_attributes_t = type_helper::values_t;
	using parse_begin_handler = std::function <
		sys_expected<version_enum>(std::string_view line_buf)
	>;
	using parse_cookie_handler = std::function <
		error_code(std::string_view line_buf)
	>;

	template <typename T>
	static constexpr bool file_opt_token_v = concepts::file_opt_token_p <
		T, char, file_optype::single, io_permission::write
	>;

public:
	explicit parser(size_t init_buf_size = 0xFFFF);
	~parser() override;

	parser(parser &&other) noexcept;
	parser &operator=(parser &&other) noexcept;

public:
	parser &on_parse_begin(parse_begin_handler func);
	parser &on_parse_cookie(parse_cookie_handler func);
	[[nodiscard]] static error_code make_error_code(parse_errno errc);

	sys_expected<bool> append(const const_buffer &buf);
	parser &operator<<(const const_buffer &buf);
	parser &reset();

public:
	[[nodiscard]] std::string take_partial_body(size_t size);
	[[nodiscard]] std::string take_body();

	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] stage_t stage() const noexcept;
	[[nodiscard]] const chunk_attributes_t &chunk_attributes() const noexcept;

public:
	parser &unbind_parse_begin();
	parser &unbind_parse_cookie();

public:
	template <typename Opt>
	[[nodiscard]] static auto make_file_opt_token(Opt &&opt)
		noexcept requires file_opt_token_v<Opt>;

private:
	class impl;
	impl *m_impl;
};

using base_parser = parser<protocol_model::base>;

} //namespace libgs::http_nt
#include <libgs/http_nt/protocol/utils/core/detail/parser.h>


#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_PARSER_H
