// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_ERROR_H
#define LIBGS_WEBSOCKET_ERROR_H

#include <libgs/websocket/global.h>
#include <system_error>
#include <type_traits>

namespace libgs::websocket
{

// Errors shared by the opening-handshake codec and WebSocket operations.
// Frame wire-format violations remain in protocol_errc.
enum class errc
{
	handshake_rejected = 1,
	invalid_upgrade,
	invalid_accept_key,
	unsupported_version,
	unsupported_subprotocol,
	unsupported_extension,
	redirect_limit_exceeded,
	insecure_redirect,
	message_too_big,
	write_queue_full,
	already_open,
	not_open,
	closing,
	closed,
};

[[nodiscard]] LIBGS_WEBSOCKET_API
const std::error_category &error_category() noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
error_code make_error_code(errc value) noexcept;

} //namespace libgs::websocket

namespace std
{

template <>
struct is_error_code_enum<libgs::websocket::errc> : true_type {};

} //namespace std

#endif //LIBGS_WEBSOCKET_ERROR_H
