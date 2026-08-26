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

#include "upgrade.h"
#include <libgs/core/string_vector.h>

namespace libgs::http
{

bool header_has_token
(const headers &values, std::string_view field, std::string_view token) noexcept
{
	auto it = values.find(std::string(field));
	if( it == values.end() )
		return false;

	auto wanted = strtls::to_lower(strtls::trimmed(token));
	return std::ranges::any_of (
		string_vector::from_string(it->second.to_string(), ','),
		[&](const auto &item) {
			return strtls::to_lower(strtls::trimmed(item)) == wanted;
		}
	);
}

std::optional<std::string> upgrade_protocol(const headers &values) noexcept
{
	auto it = values.find(header::upgrade);
	if( it == values.end() )
		return std::nullopt;

	auto value = strtls::trimmed(it->second.to_string());
	if( value.empty() )
		return std::nullopt;
	return value;
}

bool is_upgrade_request(const headers &values) noexcept
{
	return header_has_token(values, header::connection, "upgrade") and
		   upgrade_protocol(values).has_value();
}

bool is_upgrade_response(status_enum status, const headers &values) noexcept
{
	return status == status::switching_protocols and is_upgrade_request(values);
}

} //namespace libgs::http
