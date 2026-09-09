// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_SECURE_RANDOM_H
#define LIBGS_WEBSOCKET_DETAIL_SECURE_RANDOM_H

#include <libgs/websocket/global.h>

namespace libgs::websocket::detail
{

// Fills the complete output from the operating system CSPRNG. No deterministic
// or time-based fallback is permitted: source failure is returned to the caller.
[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<>
secure_random_bytes(const mutable_buffer &output) noexcept;

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_SECURE_RANDOM_H
