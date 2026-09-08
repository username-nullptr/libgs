// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_PROTOCOL_HANDSHAKE_H
#define LIBGS_WEBSOCKET_PROTOCOL_HANDSHAKE_H

#include <libgs/websocket/error.h>
#include <libgs/websocket/protocol/types.h>
#include <libgs/http/protocol/types.h>

namespace libgs::websocket
{

struct opening_request
{
	std::string key {};
	std::vector<std::string> subprotocols {};
	// Parsed for forward compatibility. Baseline client configuration must leave
	// this empty; a baseline server may parse and ignore a valid peer offer.
	std::vector<extension> extensions {};
};

struct opening_response
{
	std::optional<std::string> subprotocol {};
	// Reserved for post-baseline negotiation. A baseline response must be empty,
	// and a baseline client rejects a non-empty peer response.
	std::vector<extension> extensions {};
};

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<std::string>
make_client_key(std::span<const std::byte,16> nonce) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<std::string>
make_accept_key(std::string_view client_key) noexcept;

// Host belongs to the URL/HTTP adapter and is intentionally not generated here.
[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<http::headers>
make_opening_request_headers(const opening_request &request) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<opening_request>
parse_opening_request (
	http::method_enum method,
	http::version_enum version,
	const http::headers &headers
) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<http::headers>
make_opening_response_headers (
	const opening_request &request,
	const opening_response &response
) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<opening_response>
parse_opening_response (
	http::status_enum status,
	const http::headers &headers,
	const opening_request &request
) noexcept;

} //namespace libgs::websocket

#endif //LIBGS_WEBSOCKET_PROTOCOL_HANDSHAKE_H
