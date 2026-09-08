// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "error.h"

namespace libgs::websocket { namespace
{

class websocket_error_category final : public std::error_category
{
public:
	[[nodiscard]] const char *name() const noexcept override {
		return "libgs::websocket";
	}

	[[nodiscard]] std::string message(int code) const override
	{
		switch(static_cast<errc>(code))
		{
		case errc::handshake_rejected:
			return "WebSocket handshake rejected";
		case errc::invalid_upgrade:
			return "Invalid WebSocket upgrade";
		case errc::invalid_accept_key:
			return "Invalid WebSocket accept key";
		case errc::unsupported_version:
			return "Unsupported WebSocket version";
		case errc::unsupported_subprotocol:
			return "Unsupported WebSocket subprotocol";
		case errc::unsupported_extension:
			return "Unsupported WebSocket extension";
		case errc::redirect_limit_exceeded:
			return "WebSocket redirect limit exceeded";
		case errc::insecure_redirect:
			return "Insecure WebSocket redirect";
		case errc::message_too_big:
			return "WebSocket message too big";
		case errc::write_queue_full:
			return "WebSocket write queue full";
		case errc::already_open:
			return "WebSocket stream already open";
		case errc::not_open:
			return "WebSocket stream is not open";
		case errc::closing:
			return "WebSocket stream is closing";
		case errc::closed:
			return "WebSocket stream is closed";
		default:
			return "Unknown WebSocket error";
		}
	}
};

} //namespace

const std::error_category &error_category() noexcept
{
	static websocket_error_category category;
	return category;
}

error_code make_error_code(errc value) noexcept
{
	return { static_cast<int>(value), error_category() };
}

} //namespace libgs::websocket
