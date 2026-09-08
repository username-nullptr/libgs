// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_PROTOCOL_PARSER_H
#define LIBGS_WEBSOCKET_PROTOCOL_PARSER_H

#include <libgs/websocket/protocol/types.h>

namespace libgs::websocket
{

struct frame_parse_result
{
	// Bytes consumed from the input, including frame header bytes.
	size_t consumed = 0;
	// Borrowed slice of this call's input. Payload remains wire-masked.
	mutable_buffer payload {};
	// Offset of payload[0] in the current frame payload.
	uint64_t payload_offset = 0;
	// One-shot event: this call completed the current frame header.
	bool header_ready = false;
	// One-shot event: this call completed the current frame.
	bool frame_finished = false;
};

[[nodiscard]] LIBGS_WEBSOCKET_API sys_expected<close_payload_view>
decode_close_payload(const const_buffer &payload) noexcept;

// Incremental frame parser with fragmentation sequencing.
class LIBGS_WEBSOCKET_API frame_parser
{
	LIBGS_DISABLE_COPY(frame_parser)

public:
	// The role is intentionally explicit because it determines inbound masking.
	explicit frame_parser(frame_codec_config config);
	~frame_parser();

	frame_parser(frame_parser &&other) noexcept;
	frame_parser &operator=(frame_parser &&other) noexcept;

public:
	[[nodiscard]] sys_expected<frame_parse_result>
	parse(const mutable_buffer &input) noexcept;

	[[nodiscard]] const frame_header &header() const noexcept;

	[[nodiscard]] uint64_t payload_remaining() const noexcept;
	[[nodiscard]] frame_codec_config config() const noexcept;

	[[nodiscard]] bool failed() const noexcept;
	[[nodiscard]] error_code last_error() const noexcept;

	// Clears the partial frame, failure and cross-frame fragmentation state.
	frame_parser &reset() noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::websocket

#endif //LIBGS_WEBSOCKET_PROTOCOL_PARSER_H
