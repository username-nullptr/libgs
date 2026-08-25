
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_CONDITIONAL_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_CONDITIONAL_H

#include <libgs/http_nt/protocol/header.h>
#include <libgs/http_nt/protocol/types.h>

namespace libgs::http_nt
{

struct LIBGS_HTTP_NT_API entity_tag
{
	bool weak = false;
	std::string opaque {};
};

enum class precondition_result
{
	proceed,
	not_modified,
	precondition_failed
};

[[nodiscard]] LIBGS_HTTP_NT_API optional<entity_tag>
parse_entity_tag(std::string_view value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API bool
strong_entity_tag_equal(std::string_view lhs, std::string_view rhs) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API bool
weak_entity_tag_equal(std::string_view lhs, std::string_view rhs) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API optional<std::chrono::system_clock::time_point>
parse_http_date(std::string_view value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API std::string
format_http_date(std::chrono::system_clock::time_point value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API precondition_result evaluate_preconditions (
	method_enum method, const headers &request_headers, const headers &representation_headers,
	bool representation_exists = true
) noexcept;

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_CONDITIONAL_H
