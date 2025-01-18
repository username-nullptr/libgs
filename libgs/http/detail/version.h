
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

#ifndef LIBGS_HTTP_DETAIL_VERSION_H
#define LIBGS_HTTP_DETAIL_VERSION_H

namespace libgs::http
{

inline bool version_check(version_t v, bool _throw)
{
	switch(v)
	{
#define X_MACRO(e,v,s) case http::version::e:
		LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
			return true;
		default:
			if( _throw )
				throw runtime_error("libgs::http::version_check: Invalid http version: '{}'.", v);
			break;
	}
	return false;
}

inline bool version_check(std::string_view vs, bool _throw)
{
#define X_MACRO(e,v,s) \
	if( vs == s ) \
		return true;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else if( _throw )
		throw runtime_error("libgs::http::version_check: Invalid http version: '{}'.", vs);
	return false;
}

template <version_t Version, core_concepts::char_type CharT>
consteval const CharT *version_string()
{
#define X_MACRO(e,v,s) \
	if constexpr( Version == version::e ) { \
		if constexpr( is_char_v<CharT> ) \
			return s; \
		else \
			return LIBGS_WCHAR(s); \
	}
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else {
		static_assert(false, "Invalid http version.");
		return {};
	}
}

template <version_t Version>
consteval const char *version_string()
{
	return version_string<Version,char>();
}

template <version_t Version>
consteval const wchar_t *wversion_string()
{
	return version_string<Version,wchar_t>();
}

template <core_concepts::char_type CharT>
const CharT *version_string(version_t v)
{
	if constexpr( is_char_v<CharT> )
		return version_string(v);
	else
		return wversion_string(v);
}

inline const char *version_string(version_t v)
{
	switch(v)
	{
#define X_MACRO(e,v,s) case version::e: return s;
		LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http version: '{}'.", v);
//	return "";
}

inline const wchar_t *wversion_string(version_t v)
{
	switch(v)
	{
#define X_MACRO(e,v,s) case version::e: return LIBGS_WCHAR(s);
		LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http version: '{}'.", v);
	// return L"";
}

LIBGS_HTTP_TAPI version_t version_number(core_concepts::string_type auto &&vs, bool _throw)
{
#define X_MACRO(e,v,s) \
	if( vs == s ) \
		return version::e;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else if( _throw )
		throw runtime_error("libgs::http::version_check: Invalid http version: '{}'.", vs);
	return {};
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_VERSION_H