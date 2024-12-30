
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_DETAIL_STRING_TOOLS_H
#define LIBGS_CORE_CXX_DETAIL_STRING_TOOLS_H

#include <libgs/core/cxx/return_tools.h>
#include <memory>

namespace libgs
{

decltype(auto) nosview(concepts::string_type auto &&str)
{
	using Str = decltype(str);
	using str_t = std::remove_cvref_t<Str>;

	if constexpr( std::is_same_v<str_t, std::string_view> )
		return std::string(str);
	else if constexpr( std::is_same_v<str_t, std::wstring_view> )
		return std::wstring(str);
	else
		return return_reference(std::forward<Str>(str));
}

inline std::string wcstombs(std::wstring_view str)
{
	if( str.empty() )
		return "";

	auto size = str.size();
	auto buf = std::make_shared<char[]>(size + 1);

#ifdef _MSC_VER
	::wcstombs_s(&size, buf.get(), size, str.data(), size);
#else
	std::wcstombs(buf.get(), str.data(), size);
#endif
	return buf.get();
}

inline char wcstombs(wchar_t c)
{
	char buf = 0;
#ifdef _MSC_VER
	size_t size = 0;
	::wcstombs_s(&size, &buf, 1, &c, 1);
#else
	std::wcstombs(&buf, &c, 1);
#endif
	return buf;
}

inline std::wstring mbstowcs(std::string_view str)
{
	if( str.empty() )
		return L"";

	auto size = str.size();
	auto buf = std::make_shared<wchar_t[]>(size + 1);

#ifdef _MSC_VER
	::mbstowcs_s(&size, buf.get(), size, str.data(), size);
#else
	std::mbstowcs(buf.get(), str.data(), size);
#endif
	return buf.get();
}

inline wchar_t mbstowcs(char c)
{
	wchar_t buf = 0;
#ifdef _MSC_VER
	size_t size = 0;
	::mbstowcs_s(&size, &buf, 1, &c, 1);
#else
	std::mbstowcs(&buf, &c, 1);
#endif
	return buf;
}

decltype(auto) xxtombs(concepts::string_type auto &&str)
{
	using Str = decltype(str);
	if constexpr( is_char_string_v<Str> )
		return return_reference(std::forward<Str>(str));
	else
		return wcstombs(str);
}

char xxtombs(concepts::char_type auto c)
{
	if constexpr( is_char_v<decltype(c)> )
		return c;
	else
		return wcstombs(c);
}

decltype(auto) xxtowcs(concepts::string_type auto &&str)
{
	using Str = decltype(str);
	if constexpr( is_wchar_string_v<Str> )
		return return_reference(std::forward<Str>(str));
	else
		return mbstowcs(str);
}

wchar_t xxtowcs(concepts::char_type auto c)
{
	if constexpr( is_char_v<decltype(c)> )
		return mbstowcs(c);
	else
		return c;
}

template <concepts::char_type CharT>
decltype(auto) mbstoxx(concepts::char_string_type auto &&str)
{
	if constexpr( is_char_v<CharT> )
		return std::forward<decltype(str)>(str);
	else
		return mbstowcs(str);
}

template <concepts::char_type CharT>
CharT mbstoxx(char c)
{
	if constexpr( is_char_v<CharT> )
		return c;
	else
		return mbstowcs(c);
}

template <concepts::char_type CharT>
decltype(auto) wcstoxx(concepts::wchar_string_type auto &&str)
{
	if constexpr( is_wchar_v<CharT> )
		return std::forward<decltype(str)>(str);
	else
		return wcstombs(str);
}

template <concepts::char_type CharT>
CharT wcstoxx(wchar_t c)
{
	if constexpr( is_char_v<CharT> )
		return wcstombs(c);
	else
		return c;
}

inline string_wrapper::string_wrapper(const char *value) :
	value(value)
{

}

inline string_wrapper::string_wrapper(const wchar_t *value) :
	value(wcstombs(value))
{

}

inline string_wrapper::string_wrapper(const std::string &value) :
	value(value)
{

}

inline string_wrapper::string_wrapper(std::string &&value) :
	value(std::move(value))
{

}

inline string_wrapper::string_wrapper(std::string_view value) :
	value(value.data(), value.size())
{

}

inline string_wrapper::string_wrapper(std::wstring_view value) :
	value(wcstombs(value))
{

}

inline string_wrapper::operator std::string &()
{
	return value;
}

inline string_wrapper::operator const std::string &() const
{
	return value;
}

inline std::string &string_wrapper::operator*()
{
	return value;
}

inline std::string *string_wrapper::operator->()
{
	return &value;
}

inline bool is_alpha(std::string_view str) noexcept
{
	for(auto &c : str)
	{
		if( not std::isalpha(c) )
			return false;
	}
	return true;
}

inline bool is_digit(std::string_view str) noexcept
{
	for(auto &c : str)
	{
		if( not std::isdigit(c) )
			return false;
	}
	return true;
}

inline bool is_alnum(std::string_view str) noexcept
{
	for(auto &c : str)
	{
		if( not std::isalnum(c) )
			return false;
	}
	return true;
}

inline bool is_ascii(std::string_view str) noexcept
{
	for(auto &c : str)
	{
		if( c > 127 )
			return false;
	}
	return true;
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STRING_TOOLS_H