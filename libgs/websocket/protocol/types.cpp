// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "types.h"

namespace libgs::websocket { namespace
{

class websocket_protocol_error_category final : public std::error_category
{
public:
	[[nodiscard]] const char *name() const noexcept override {
		return "libgs::websocket::protocol";
	}

	[[nodiscard]] std::string message(int code) const override
	{
		switch(static_cast<protocol_errc>(code))
		{
		case protocol_errc::reserved_opcode:
			return "Reserved WebSocket opcode";
		case protocol_errc::unexpected_rsv:
			return "Unexpected WebSocket reserved bit";
		case protocol_errc::unexpected_mask:
			return "Unexpected WebSocket masking key";
		case protocol_errc::missing_mask:
			return "Missing WebSocket masking key";
		case protocol_errc::noncanonical_length:
			return "Non-canonical WebSocket payload length";
		case protocol_errc::invalid_64bit_length:
			return "Invalid 64-bit WebSocket payload length";
		case protocol_errc::fragmented_control_frame:
			return "Fragmented WebSocket control frame";
		case protocol_errc::control_payload_too_large:
			return "WebSocket control payload too large";
		case protocol_errc::frame_too_large:
			return "WebSocket frame too large";
		case protocol_errc::invalid_close_payload:
			return "Invalid WebSocket Close payload";
		case protocol_errc::invalid_utf8:
			return "Invalid WebSocket UTF-8";
		case protocol_errc::unexpected_continuation:
			return "Unexpected WebSocket continuation frame";
		case protocol_errc::data_during_fragmentation:
			return "WebSocket data frame during fragmented message";
		default:
			return "Unknown WebSocket protocol error";
		}
	}
};

} //namespace

const std::error_category &protocol_error_category() noexcept
{
	static websocket_protocol_error_category category;
	return category;
}

error_code make_error_code(protocol_errc value) noexcept
{
	return { static_cast<int>(value), protocol_error_category() };
}

bool operator==(const error_code &error, protocol_errc value) noexcept
{
	return error.category() == protocol_error_category() and
		error.value() == static_cast<int>(value);
}

bool operator==(protocol_errc value, const error_code &error) noexcept
{
	return error.category() == protocol_error_category() and
		error.value() == static_cast<int>(value);
}

bool operator!=(const error_code &error, protocol_errc value) noexcept
{
	return not operator==(error, value);
}

bool operator!=(protocol_errc value, const error_code &error) noexcept
{
	return not operator==(value, error);
}

bool is_known_opcode(opcode value) noexcept
{
	switch(value)
	{
	case opcode::continuation:
	case opcode::text:
	case opcode::binary:
	case opcode::close:
	case opcode::ping:
	case opcode::pong:
		return true;
	default:
		return false;
	}
}

bool is_control_opcode(opcode value) noexcept
{
	return value == opcode::close or value == opcode::ping or value == opcode::pong;
}

bool is_data_opcode(opcode value) noexcept
{
	return value == opcode::text or value == opcode::binary;
}

bool is_valid_close_code(uint16_t value) noexcept
{
	if( value >= 3000 and value < 5000 )
		return true;

	switch(static_cast<close_code>(value))
	{
	case close_code::normal_closure:
	case close_code::going_away:
	case close_code::protocol_error:
	case close_code::unsupported_data:
	case close_code::invalid_payload:
	case close_code::policy_violation:
	case close_code::message_too_big:
	case close_code::mandatory_extension:
	case close_code::internal_error:
	case close_code::service_restart:
	case close_code::try_again_later:
	case close_code::bad_gateway:
		return true;
	default:
		return false;
	}
}

close_code close_code_for(protocol_errc value) noexcept
{
	switch(value)
	{
	case protocol_errc::invalid_utf8:
		return close_code::invalid_payload;
	case protocol_errc::frame_too_large:
		return close_code::message_too_big;
	default:
		return close_code::protocol_error;
	}
}

} //namespace libgs::websocket
