// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_PERMESSAGE_DEFLATE_H
#define LIBGS_WEBSOCKET_DETAIL_PERMESSAGE_DEFLATE_H

#include <libgs/websocket/types.h>

namespace libgs::websocket::detail
{

[[nodiscard]] LIBGS_WEBSOCKET_API bool
supported_extension_set(std::span<const extension> extensions) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<std::vector<std::byte>>
deflate_message(std::span<const const_buffer> buffers) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<std::vector<std::byte>>
inflate_message(std::span<const std::byte> payload, size_t max_message_size) noexcept;

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_PERMESSAGE_DEFLATE_H
