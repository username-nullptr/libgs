
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

/*
#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *root       = __VA_ARGS__##"/"                        ; \
	static constexpr const _type *close      = __VA_ARGS__##"close"                    ; \
	static constexpr const _type *gzip       = __VA_ARGS__##"gzip"                     ; \
	static constexpr const _type *chunked    = __VA_ARGS__##"chunked"                  ; \
	static constexpr const _type *upgrade    = __VA_ARGS__##"upgrade"                  ; \
	static constexpr const _type *websocket  = __VA_ARGS__##"websocket"                ; \
	static constexpr const _type *set_cookie = __VA_ARGS__##"set-cookie"               ; \
	static constexpr const _type *text_plain = __VA_ARGS__##"text/plain; charset=utf-8";
*/

inline bool status::check(enumeration status, bool _throw)
{
	switch(status)
	{
#define X_MACRO(e,v,d) case e:
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
			return true;
		default:
			if( _throw )
			{
				throw runtime_error (
					"libgs::http::status::check: Invalid http status: '{}'.",
					status
				);
			}
			break;
	}
	return false;
}

template <status_enum Status>
consteval bool status::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Status == e ) return true;
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else return false;
}

inline const char *status::description(enumeration status, bool _throw)
{
	switch(status)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::status::description: Invalid http status: '{}'.",
				status
			);
		}
		break;
	}
	return "";
}

template <status_enum Status>
consteval const char *status::description() requires is_valid_v<Status>
{
#define X_MACRO(e,v,d) if constexpr( Status == e ) return d;
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else return "";
}

inline bool method::check(enumeration method, bool _throw)
{
	switch(method)
	{
#define X_MACRO(e,v,d) case e:
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
			return true;
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::method::check: Invalid http method: '{}'.",
				method
			);
		}
		break;
	}
	return false;
}

template <method_enum Method>
consteval bool method::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Method == e ) return true;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else return false;
}

inline const char *method::string(enumeration method, bool _throw)
{
	switch(method)
	{
#define X_MACRO(e,v,d) case e: return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::method::string: Invalid http method: '{}'.",
				method
			);
		}
		break;
	}
	return "";
}

template <method_enum Method>
consteval const char *method::string() requires is_valid_v<Method>
{
#define X_MACRO(e,v,d) if constexpr( Method == e ) return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else return "";
}

constexpr method_enum method::from_string(std::string_view str)
{
#define X_MACRO(e,v,d) if( str == d ) return method::e;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	throw runtime_error (
		"libgs::http::method::from_string: Invalid http method: '{}'.", str
	);
}

constexpr method::method(std::string_view str) :
	value(from_string(str))
{

}

inline bool redirect::check(enumeration redirect, bool _throw)
{
	switch(redirect)
	{
#define X_MACRO(e,v,d) case e:
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		return true;
	default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::redirect::check: Invalid http redirect type: '{}'.",
				redirect
			);
		}
		break;
	}
	return "";
}

template <redirect_enum Redirect>
consteval bool redirect::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Redirect == e ) return true;
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
	else return false;
}

inline const char *redirect::description(enumeration redirect, bool _throw)
{
	switch(redirect)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
	#undef X_MACRO
		default:
		if( _throw )
		{
			throw runtime_error (
				"libgs::http::redirect::string: Invalid http redirect type: '{}'.",
				redirect
			);
		}
		break;
	}
	return "";
}

template <redirect_enum Redirect>
consteval const char *redirect::description() requires is_valid_v<Redirect>
{
#define X_MACRO(e,v,d) if constexpr( Redirect == e ) return d;
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
	else return "";
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_TYPES_H
