
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_RANGE_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_RANGE_H

#include <libgs/http_nt/protocol/utils/core/body_norms.h>
#include <libgs/http_nt/protocol/header.h>

namespace libgs::http_nt
{

struct LIBGS_HTTP_NT_API byte_range_spec
{
	enum class form_t {
		closed, open_ended, suffix
	};
	form_t form = form_t::closed;
	size_t first = 0;
	size_t last = 0;
};

struct LIBGS_HTTP_NT_API ranges_specifier
{
	std::string unit {};
	std::vector<byte_range_spec> ranges {};
};

struct LIBGS_HTTP_NT_API content_range
{
	std::string unit {};
	bool satisfied = false;

	size_t first = 0;
	size_t last = 0;
	optional<size_t> complete_length {};

	[[nodiscard]] size_t length() const noexcept;
};

struct LIBGS_HTTP_NT_API byte_range_part
{
	http_nt::headers fields {};
	content_range range {};
};

struct LIBGS_HTTP_NT_API byte_range_chunk
{
	size_t part_index = 0;
	size_t offset = 0;
	std::string data {};
};

[[nodiscard]] LIBGS_HTTP_NT_API sys_expected<ranges_specifier>
parse_range_header(std::string_view value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API file_ranges
resolve_byte_ranges(const ranges_specifier &specifier, size_t complete_length) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API sys_expected<content_range>
parse_content_range(std::string_view value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API std::string
format_content_range(const file_range &range, size_t complete_length);

[[nodiscard]] LIBGS_HTTP_NT_API std::string
format_unsatisfied_content_range(size_t complete_length);

[[nodiscard]] LIBGS_HTTP_NT_API sys_expected<std::string>
parse_multipart_byte_ranges_boundary(std::string_view content_type) noexcept;

class LIBGS_HTTP_NT_API multipart_byte_ranges_parser final
{
	LIBGS_DISABLE_COPY(multipart_byte_ranges_parser)

public:
	explicit multipart_byte_ranges_parser(std::string boundary);
	~multipart_byte_ranges_parser();

	multipart_byte_ranges_parser(multipart_byte_ranges_parser &&other) noexcept;
	multipart_byte_ranges_parser &operator=(multipart_byte_ranges_parser &&other) noexcept;

	[[nodiscard]] sys_expected<std::vector<byte_range_chunk>>
	append(std::string_view data) noexcept;

	[[nodiscard]] error_code finish() const noexcept;
	[[nodiscard]] bool finished() const noexcept;
	[[nodiscard]] const std::vector<byte_range_part> &parts() const noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_RANGE_H
