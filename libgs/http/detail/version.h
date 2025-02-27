
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

namespace detail
{

template <version_t>
struct version_string;

#define X_MACRO(e,v,s) \
	template <> struct version_string<version::e> { \
		constexpr const char *value = s; \
	};
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO

} //namespace detail

template <version_t Version>
consteval const char *version_string()
{
	return detail::version_string<Version>::value;
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

version_t version_number(const core_concepts::string_type auto &vs, bool _throw)
{
#define X_MACRO(e,v,s) if( vs == s ) return version::e;
	LIBGS_HTTP_VERSION_TABLE
#undef X_MACRO
	else if( _throw )
		throw runtime_error("libgs::http::version_check: Invalid http version: '{}'.", vs);
	return {};
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_VERSION_H