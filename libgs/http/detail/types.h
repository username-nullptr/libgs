
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

#ifndef LIBGS_HTTP_DETAIL_TYPES_H
#define LIBGS_HTTP_DETAIL_TYPES_H

namespace libgs::http
{

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *root       = __VA_ARGS__##"/"                        ; \
	static constexpr const _type *close      = __VA_ARGS__##"close"                    ; \
	static constexpr const _type *gzip       = __VA_ARGS__##"gzip"                     ; \
	static constexpr const _type *chunked    = __VA_ARGS__##"chunked"                  ; \
	static constexpr const _type *upgrade    = __VA_ARGS__##"upgrade"                  ; \
	static constexpr const _type *websocket  = __VA_ARGS__##"websocket"                ; \
	static constexpr const _type *set_cookie = __VA_ARGS__##"set-cookie"               ; \
	static constexpr const _type *text_plain = __VA_ARGS__##"text/plain; charset=utf-8";




inline bool status_check(status_t s, bool _throw)
{
	switch(s)
	{
#define X_MACRO(e,v,d) case http::status::e:
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
			return true;
		default:
			if( _throw )
				throw runtime_error("libgs::http::status_check: Invalid http status: '{}'.", s);
			break;
	}
	return false;
}

inline void method_check(uint32_t m)
{
	method_check(static_cast<method>(m));
}

inline bool method_check(method m, bool _throw)
{
	switch(m)
	{
#define X_MACRO(e,v,d) case http::method::e:
		LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
			return true;
		default:
			if( _throw )
				throw runtime_error("libgs::http::method_check: Invalid http method: '{}'.", m);
			break;
	}
	return false;
}

inline bool redirect_check(redirect type, bool _throw)
{
	switch(type)
	{
#define X_MACRO(e,v) case redirect::e:
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
			return true;
		default:
			if( _throw )
				throw runtime_error("libgs::http::redirect_check: Invalid redirect type: '{}'.", type);
			break;
	}
	return false;
}

template <status_t Status>
consteval const char *status_description()
{
#define X_MACRO(e,v,d) Status == status::e and
	static_assert(LIBGS_HTTP_STATUS_TABLE true, "Invalid http status.");
#undef X_MACRO

#define X_MACRO(e,v,d) \
	if constexpr( Status == status::e ) return d;
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else
		return "";
}

inline const char *status_description(status_t s)
{
	switch(s)
	{
#define X_MACRO(e,v,d) case status::e: return d;
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http status: '{}'.", s);
//	return "";
}


template <method Method>
consteval const char *method_string()
{
#define X_MACRO(e,v,d) Method == method::e and
	static_assert(LIBGS_HTTP_METHOD_TABLE true, "Invalid http method.");
#undef X_MACRO

#define X_MACRO(e,v,d) \
if constexpr( Method == method::e ) return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else
		return "";
}

inline const char *method_string(method m)
{
	switch(m)
	{
#define X_MACRO(e,v,d) case method::e: return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http method: '{}'.", m);
//	return "";
}

inline method from_method_string(std::string_view str)
{
#define X_MACRO(e,v,d) if( str == d ) return method::e;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	throw runtime_error("libgs::http: Invalid http method: '{}'.", str);
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_TYPES_H
