
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_TYPES_H
#define LIBGS_HTTP_BASIC_DETAIL_TYPES_H

namespace libgs::http
{

inline void status_check(uint32_t status)
{
	switch(static_cast<http::status>(status))
	{
#define X_MACRO(e,v,d) case http::status::e:
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
			break;
		default:
			throw runtime_error("libgs::http::status_check: Invalid http status: '{}'.", status);
	}
}

inline void method_check(uint32_t method)
{
	switch(static_cast<http::method>(method))
	{
#define X_MACRO(e,v,d) case http::method::e:
		LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
			break;
		default:
			throw runtime_error("libgs::http::method_check: Invalid http method: '{}'.", method);
	}
}

inline void redirect_type_check(uint32_t type)
{
	switch(static_cast<http::redirect_type>(type))
	{
#define X_MACRO(e,v) case redirect_type::e:
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
			break;
		default:
			throw runtime_error("libgs::http::redirect_type_check: Invalid redirect type: '{}'.", type);
	}
}

template <concept_char_type CharT>
std::basic_string<CharT> to_status_description(status s)
{
	switch(s)
	{
#define X_MACRO(e,v,d) case status::e: return d;
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http status (enum): '{}'.", s);
//	return {};
}

template <concept_char_type CharT>
std::basic_string<CharT> to_method_string(method m)
{
	switch(m)
	{
#define X_MACRO(e,v,d) case method::e: return d;
		LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
		default: break;
	}
	throw runtime_error("libgs::http: Invalid http method (enum): '{}'.", m);
//	return {};
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


#endif //LIBGS_HTTP_BASIC_DETAIL_TYPES_H
