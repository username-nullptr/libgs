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
#define X_MACRO(e,v,d) case errc::e: return d;
		LIBGS_WEBSOCKET_ERRC_TABLE
#undef X_MACRO
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

bool operator==(const error_code &error, errc value) noexcept
{
	return error.category() == error_category() and
		error.value() == static_cast<int>(value);
}

bool operator==(errc value, const error_code &error) noexcept
{
	return error.category() == error_category() and
		error.value() == static_cast<int>(value);
}

bool operator!=(const error_code &error, errc value) noexcept
{
	return not operator==(error, value);
}

bool operator!=(errc value, const error_code &error) noexcept
{
	return not operator==(value, error);
}

} //namespace libgs::websocket
