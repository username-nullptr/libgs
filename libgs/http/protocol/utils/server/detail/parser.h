// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_SERVER_DETAIL_PARSER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_SERVER_DETAIL_PARSER_H

namespace libgs::http
{

optional<value> parser<protocol_model::server>::path_arg
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = path_args().find(strtls::to_view(key));
	return it == path_args().end() ?
		optional<value>() : libgs::make_optional(it->second);
}

} //namespace libgs::http


#endif //LIBGS_HTTP_PROTOCOL_UTILS_SERVER_DETAIL_PARSER_H
