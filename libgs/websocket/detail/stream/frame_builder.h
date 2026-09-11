// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_BUILDER_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_BUILDER_H

#include <libgs/websocket/types.h>

namespace libgs::websocket::detail
{

struct prepared_frame
{
	std::shared_ptr<std::vector<std::byte>> wire;
	std::shared_ptr<std::vector<std::byte>> payload_owner;
	std::vector<const_buffer> buffers;

	size_t header_size = 0;
	size_t payload_size = 0;
	size_t application_size = 0;
};

// Owns outbound frame construction policy. It deliberately has no connection
// or executor dependency, so validation, fragmentation and masking remain
// separate from the stream's transport lifecycle.
class LIBGS_WEBSOCKET_API frame_builder
{
public:
	frame_builder() noexcept = default;

	frame_builder(role local_role, const stream_config &config,
		std::span<const extension> extensions = {}) noexcept;

	frame_builder &reset(role local_role, const stream_config &config,
		std::span<const extension> extensions = {}) noexcept;

	[[nodiscard]] sys_expected<prepared_frame> prepare_control (
		opcode op, const const_buffer &payload, bool borrow_payload = false
	) const noexcept;

	[[nodiscard]] sys_expected<prepared_frame> prepare_close (
		const close_frame &frame
	) const noexcept;

	[[nodiscard]] sys_expected<std::vector<prepared_frame>> prepare_message (
		message_type type, std::span<const const_buffer> buffers
	) const noexcept;

private:
	role m_role = role::client;
	size_t m_max_frame_size = stream_config{}.max_frame_size;
	size_t m_max_message_size = stream_config{}.max_message_size;
	size_t m_fragment_size = stream_config{}.write_fragment_size;
	bool m_permessage_deflate = false;
};

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_BUILDER_H
