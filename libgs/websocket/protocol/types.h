// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_PROTOCOL_TYPES_H
#define LIBGS_WEBSOCKET_PROTOCOL_TYPES_H

#include <libgs/websocket/global.h>
#include <libgs/core/utils/flags.h>

namespace libgs::websocket
{

enum class role : uint8_t {
	client, server,
};

// Normalized RSV bits. These values intentionally use the low three bits;
// they are shifted to/from the wire header by the frame codec.
enum class reserved_bit : uint8_t
{
	rsv1 = 0x04,
	rsv2 = 0x02,
	rsv3 = 0x01,
};
LIBGS_DECLARE_FLAGS(reserved_bits,reserved_bit);
LIBGS_DECLARE_OPERATORS_FOR_FLAGS(reserved_bits);

struct extension_parameter
{
	std::string name {};
	// RFC 6455 defines an extension value as token or quoted-string. Numeric
	// meaning, when any, belongs to the concrete extension codec.
	optional<std::string> value {};
};

struct extension
{
	std::string name {};
	std::vector<extension_parameter> parameters {};
};

// LibGS currently implements the RFC 7692 profile that disables context
// takeover in both directions. The descriptor returned here is suitable for
// connect_request::extensions and upgrade_options::supported_extensions.
[[nodiscard]] LIBGS_WEBSOCKET_API extension
permessage_deflate_extension();

[[nodiscard]] LIBGS_WEBSOCKET_API bool
is_permessage_deflate_extension(const extension &value) noexcept;

struct frame_codec_config
{
	role local_role = role::client;

	// Zero permits every payload length representable by RFC 6455.
	uint64_t max_frame_size = 16 * 1024 * 1024;

	// Extension codecs set the bits they own; the default rejects all RSV bits.
	reserved_bits allowed_rsv {};
};

enum class opcode : uint8_t
{
	continuation = 0x0,
	text         = 0x1,
	binary       = 0x2,
	close        = 0x8,
	ping         = 0x9,
	pong         = 0xA,
};

enum class close_code : uint16_t
{
	normal_closure      = 1000,
	going_away          = 1001,
	protocol_error      = 1002,
	unsupported_data    = 1003,
	invalid_payload     = 1007,
	policy_violation    = 1008,
	message_too_big     = 1009,
	mandatory_extension = 1010,
	internal_error      = 1011,
	service_restart     = 1012,
	try_again_later     = 1013,
	bad_gateway         = 1014,
};

struct LIBGS_WEBSOCKET_API close_frame
{
	uint16_t code = static_cast<uint16_t>(close_code::normal_closure);
	std::string reason {};

	close_frame() = default;
	close_frame(close_code value, std::string text = {});
	close_frame(uint16_t value, std::string text = {});
};

struct close_payload_view
{
	optional<uint16_t> code {};
	std::string_view reason {};
};

struct masking_key
{
	std::array<std::byte,4> bytes {};
	[[nodiscard]] bool operator==(const masking_key&)
		const noexcept = default;
};

struct frame_header
{
	bool fin = true;
	reserved_bits rsv {};

	opcode op = opcode::binary;
	uint64_t payload_size = 0;

	optional<masking_key> mask {};
};

enum class protocol_errc
{
	reserved_opcode = 1,
	unexpected_rsv,
	unexpected_mask,
	missing_mask,
	noncanonical_length,
	invalid_64bit_length,
	fragmented_control_frame,
	control_payload_too_large,
	frame_too_large,
	invalid_close_payload,
	invalid_utf8,
	unexpected_continuation,
	data_during_fragmentation,
	invalid_compressed_payload,
};

[[nodiscard]] LIBGS_WEBSOCKET_API
const std::error_category &protocol_error_category() noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
error_code make_error_code(protocol_errc value) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
bool is_known_opcode(opcode value) noexcept;

// Recognized control opcodes only; reserved 0xB-0xF values return false.
[[nodiscard]] LIBGS_WEBSOCKET_API
bool is_control_opcode(opcode value) noexcept;

// Message-start opcodes only; continuation is handled as a separate state.
[[nodiscard]] LIBGS_WEBSOCKET_API
bool is_data_opcode(opcode value) noexcept;

// Accepts assigned protocol codes represented above and application/library
// codes in [3000,5000). Reserved sentinels such as 1005/1006/1015 are rejected.
[[nodiscard]] LIBGS_WEBSOCKET_API
bool is_valid_close_code(uint16_t value) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
close_code close_code_for(protocol_errc value) noexcept;

} //namespace libgs::websocket

namespace std
{

template <>
struct is_error_code_enum<libgs::websocket::protocol_errc> : true_type {};

} //namespace std


#endif //LIBGS_WEBSOCKET_PROTOCOL_TYPES_H
