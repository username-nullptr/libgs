// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "parser_types.h"

namespace libgs::http { namespace
{

class LIBGS_DECL_HIDDEN error_category : public std::error_category
{
	LIBGS_DISABLE_COPY_MOVE(error_category)

public:
	error_category() = default;

	[[nodiscard]] const char *name() const noexcept override {
		return "libgs::http::request_parser_error";
	}

	[[nodiscard]] std::string message(int code) const override
	{
		switch(static_cast<parse_errc>(code))
		{
#define X_MACRO(e,v,d) case parse_errc::e: return d;
			LIBGS_HTTP_PARSE_ERRC_TABLE
	#undef X_MACRO
			default: break;
		}
		return "Unknown error.";
	}
}
g_error_category;

} //namespace

const std::error_category &parse_error_category() noexcept
{
	return g_error_category;
}

error_code make_error_code(parse_errc value) noexcept
{
	return { static_cast<int>(value), parse_error_category() };
}

bool operator==(const error_code &error, parse_errc value) noexcept
{
	return error.category() == parse_error_category() and
		error.value() == static_cast<int>(value);
}

bool operator==(parse_errc value, const error_code &error) noexcept
{
	return error.category() == parse_error_category() and
		error.value() == static_cast<int>(value);
}

bool operator!=(const error_code &error, parse_errc value) noexcept
{
	return error.category() != parse_error_category() or
		error.value() != static_cast<int>(value);
}

bool operator!=(parse_errc value, const error_code &error) noexcept
{
	return error.category() != parse_error_category() or
		error.value() != static_cast<int>(value);
}

} //namespace libgs::http
