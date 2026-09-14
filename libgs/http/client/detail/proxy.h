// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_CLIENT_DETAIL_PROXY_H
#define LIBGS_HTTP_CLIENT_DETAIL_PROXY_H

#include <libgs/http/client/proxy.h>

namespace libgs::http::detail
{

struct resolved_proxy
{
	optional<url> forward {};
	optional<proxy_tunnel> tunnel {};
	optional<std::string> authorization {};
};

[[nodiscard]] LIBGS_HTTP_API sys_expected<resolved_proxy>
resolve_proxy(const url &target, const proxy_t &setting) noexcept;

// Internal entry point for protocol adapters such as WebSocket. The caller
// owns the environment-variable precedence; parsing and NO_PROXY handling stay
// shared with the HTTP client.
[[nodiscard]] LIBGS_HTTP_API sys_expected<resolved_proxy>
resolve_global_proxy(const url &target,
	std::initializer_list<std::string_view> variable_names
) noexcept;

} //namespace libgs::http::detail


#endif //LIBGS_HTTP_CLIENT_DETAIL_PROXY_H
