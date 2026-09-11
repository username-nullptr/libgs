// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_TYPES_H
#define LIBGS_WEBSOCKET_TYPES_H

#include <libgs/websocket/protocol/types.h>
#include <libgs/websocket/error.h>

namespace libgs::websocket
{

enum class message_type : uint8_t {
	text = 0x1, binary = 0x2,
};

enum class control_type : uint8_t {
	ping = 0x9, pong = 0xA,
};

enum class connection_state : uint8_t {
	idle, open, closing, closed, failed,
};

struct close_info
{
	// Empty means the peer sent an empty Close frame, or the transport ended
	// without a Close frame.
	optional<uint16_t> code {};

	std::string reason {};
	bool clean = false;
};

template <typename Buffer>
struct basic_message
{
	message_type type = message_type::binary;
	Buffer body {};
};

using message = basic_message<std::vector<std::byte>>;

struct control_event
{
	control_type type = control_type::ping;
	std::vector<std::byte> payload {};
};

struct adopt_options
{
	role stream_role = role::client;
	std::vector<std::byte> pending_data {};
	std::string negotiated_subprotocol {};

	// Reserved for post-baseline extension codecs. The baseline stream rejects
	// non-empty negotiated extensions with errc::unsupported_extension.
	std::vector<extension> negotiated_extensions {};
};

struct stream_config
{
	// Zero removes the corresponding protocol-size limit. A finite default is
	// retained so read() cannot grow an owned message without a bound.
	size_t max_frame_size = 16 * 1024 * 1024;
	size_t max_message_size = 16 * 1024 * 1024;

	// Size of the stream-owned transport read block. Zero maps to
	// std::errc::invalid_argument.
	size_t read_buffer_size = 16 * 1024;

	// These limits apply only to operations waiting behind the active transport
	// write. Zero disables backlog while still permitting one active write.
	size_t max_queued_write_bytes = 64 * 1024 * 1024;
	size_t max_queued_write_operations = 64;

	// Zero disables automatic fragmentation of outgoing data messages.
	size_t write_fragment_size = 16 * 1024;

	// When false, the application must observe Ping through wait_ctrl() and
	// promptly send a Pong with the same payload.
	bool automatic_pong = true;

	// Non-positive means the deadline has already expired. The deadline can
	// preempt asynchronous I/O; synchronous I/O checks it between blocking calls.
	std::chrono::milliseconds close_timeout {5000};
};

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_TYPES_H
