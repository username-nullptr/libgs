
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_HTTP_VERSION_H
#define LIBGS_HTTP_VERSION_H

#include <libgs/http/global.h>

namespace libgs::http
{

#define LIBGS_HTTP_VERSION_TABLE \
X_MACRO( nan , 0x0000 , "NAN" ) \
X_MACRO( v10 , 0x0100 , "1.0" ) \
X_MACRO( v11 , 0x0101 , "1.1" )
// X_MACRO( v12 , 0x0102 , "1.2" )
// X_MACRO( v20 , 0x0200 , "2.0" )

struct LIBGS_HTTP_VAPI version
{
	using type = uint16_t;
#define X_MACRO(e,v,s) static constexpr type e = (v);
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
};
using version_t = version::type;
LIBGS_HTTP_VAPI bool version_check(version_t v, bool _throw = true);
LIBGS_HTTP_VAPI bool version_check(std::string_view vs, bool _throw = true);

template <version_t Version, core_concepts::char_type CharT>
[[nodiscard]] consteval const CharT *version_string();

template <version_t Version>
[[nodiscard]] consteval const char *version_string();

template <version_t Version>
[[nodiscard]] consteval const wchar_t *wversion_string();

template <core_concepts::char_type CharT>
[[nodiscard]] LIBGS_HTTP_TAPI const CharT *version_string(version_t v);

[[nodiscard]] LIBGS_HTTP_VAPI const char *version_string(version_t v);
[[nodiscard]] LIBGS_HTTP_VAPI const wchar_t *wversion_string(version_t v);

LIBGS_HTTP_TAPI version_t version_number (
	core_concepts::string_type auto &&vs, bool _throw = true
);

} //namespace libgs::http
#include <libgs/http/detail/version.h>


#endif //LIBGS_HTTP_VERSION_H