// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_VERSION_H
#define LIBGS_HTTP_PROTOCOL_VERSION_H

#include <libgs/http/global.h>

namespace libgs::http
{

#define LIBGS_HTTP_VERSION_TABLE \
X_MACRO( none , 0x0000 , "0.0" ) \
X_MACRO( v10  , 0x0100 , "1.0" ) \
X_MACRO( v11  , 0x0101 , "1.1" )
// X_MACRO( v12  , 0x0102 , "1.2" )
// X_MACRO( v20  , 0x0200 , "2.0" )

#define X_MACRO(e,v,d) e = (v),
LIBGS_HTTP_DEFINE_ENUM(uint16_t, version, LIBGS_HTTP_VERSION_TABLE, string,
	[[nodiscard]] static constexpr enumeration from_string(std::string_view str);
	constexpr version(std::string_view str);
	[[nodiscard]] static double number(enumeration v, bool _throw = true);
	[[nodiscard]] double number(bool _throw = true) const;
	template <enumeration Enum> [[nodiscard]] static consteval double number()
		requires is_valid_v<Enum>;
);
#undef X_MACRO

} //namespace libgs::http
#include <libgs/http/protocol/detail/version.h>


#endif //LIBGS_HTTP_PROTOCOL_VERSION_H
