
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

#ifndef LIBGS_HTTP_PROTOCOL_DETAIL_VERSION_H
#define LIBGS_HTTP_PROTOCOL_DETAIL_VERSION_H

namespace libgs::http::protocol
{

inline bool version::check(enumeration version, bool _throw)
{
	switch(version)
	{
#define X_MACRO(e,v,d) case e:
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
		return true;
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::version::check: Invalid http version: '{}'.",
				version
			);
		}
		break;
	}
	return false;
}

template <version_enum Version>
consteval bool version::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Version == e ) return true;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else return false;
}

inline const char *version::string(enumeration version, bool _throw)
{
	switch(version)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::version::string: Invalid http version: '{}'.",
				version
			);
		}
		break;
	}
	return "";
}

template <version_enum Version>
consteval const char *version::string() requires is_valid_v<Version>
{
#define X_MACRO(e,v,d) if constexpr( Version == e ) return d;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else return "";
}

constexpr version_enum version::from_string(std::string_view str)
{
#define X_MACRO(e,v,d) if( str == d ) return version_enum::e;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	throw runtime_error (
		"libgs::http::version::from_string: Invalid http version string: '{}'.", str
	);
}

constexpr version::version(std::string_view str) :
	value(from_string(str))
{

}

inline double version::number(enumeration version, bool _throw)
{
	switch(version)
	{
#define X_MACRO(e,v,d) case e: \
		return static_cast<double>((v >> 8) & 0xFF) + (v & 0xFF) / 10.0;
		LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::version::number: Invalid http version: '{}'.",
				version
			);
		}
		break;
	}
	return 0.0;
}

inline double version::number(bool _throw) const
{
	return number(value, _throw);
}

template <version_enum Version>
consteval double version::number() requires is_valid_v<Version>
{
#define X_MACRO(e,v,d) \
	if constexpr( Version == e ) \
		return static_cast<double>((v >> 8) & 0xFF) + (v & 0xFF) / 10.0;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_DETAIL_VERSION_H