
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

namespace detail
{

template <core_concepts::char_type T>
struct string_pool;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *root       = __VA_ARGS__##"/"                        ; \
	static constexpr const _type *v_1_0      = __VA_ARGS__##"1.0"                      ; \
	static constexpr const _type *v_1_1      = __VA_ARGS__##"1.1"                      ; \
	static constexpr const _type *close      = __VA_ARGS__##"close"                    ; \
	static constexpr const _type *gzip       = __VA_ARGS__##"gzip"                     ; \
	static constexpr const _type *chunked    = __VA_ARGS__##"chunked"                  ; \
	static constexpr const _type *upgrade    = __VA_ARGS__##"upgrade"                  ; \
	static constexpr const _type *websocket  = __VA_ARGS__##"websocket"                ; \
	static constexpr const _type *set_cookie = __VA_ARGS__##"set-cookie"               ; \
	static constexpr const _type *text_plain = __VA_ARGS__##"text/plain; charset=utf-8";

template <>
struct string_pool<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct string_pool<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

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

template <uint32_t Status, core_concepts::char_type CharT>
constexpr const CharT *status_description()
{
#define X_MACRO(e,v,d) \
	if constexpr( Status == status::e ) { \
		if constexpr( is_char_v<CharT> ) \
			return d; \
		else \
			return LIBGS_WCHAR(d); \
	}
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else {
		static_assert(false, "Invalid http status.");
		return {};
	}
}

template <uint32_t Status>
constexpr const char *status_description()
{
	return status_description<Status, char>();
}

template <uint32_t Status>
constexpr const wchar_t *wstatus_description()
{
	return status_description<Status, wchar_t>();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> status_description(status_t s)
{
	if constexpr( is_char_v<CharT> )
		return status_description(s);
	else
		return wstatus_description(s);
}

inline std::string status_description(status_t s)
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

inline std::wstring wstatus_description(status_t s)
{
	switch(s)
	{
#define X_MACRO(e,v,d) case status::e: return LIBGS_WCHAR(d);
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http status: '{}'.", s);
//	return L"";
}

template <method Method, core_concepts::char_type CharT>
constexpr const CharT *method_string()
{
#define X_MACRO(e,v,d) \
	if constexpr( Method == method::e ) { \
		if constexpr( is_char_v<CharT> ) \
			return d; \
		else \
			return LIBGS_WCHAR(d); \
	}
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else {
		static_assert(false, "Invalid http method.");
		return {};
	}
}

template <method Method>
constexpr const char *method_string()
{
	return method_string<Method, char>();
}

template <method Method>
constexpr const wchar_t *wmethod_string()
{
	return method_string<Method, wchar_t>();
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> method_string(method m)
{
	if constexpr( is_char_v<CharT> )
		return method_string(m);
	else
		return wmethod_string(m);
}

inline std::string method_string(method m)
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

inline std::wstring wmethod_string(method m)
{
	switch(m)
	{
#define X_MACRO(e,v,d) case method::e: return LIBGS_WCHAR(d);
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http method: '{}'.", m);
//	return L"";
}

template <core_concepts::char_type CharT>
method from_method_string(std::basic_string_view<CharT> str)
{
	if constexpr( is_char_v<CharT> )
		return from_method_string(static_cast<std::string_view>(str));
	else
		return from_method_string(static_cast<std::wstring_view>(str));
}

inline method from_method_string(std::string_view str)
{
#define X_MACRO(e,v,d) if( str == d ) return method::e;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	throw runtime_error("libgs::http: Invalid http method: '{}'.", str);
}

inline method from_method_string(std::wstring_view str)
{
#define X_MACRO(e,v,d) if( str == LIBGS_WCHAR(d) ) return method::e;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	throw runtime_error("libgs::http: Invalid http method: '{}'.", wcstombs(str));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_TYPES_H
