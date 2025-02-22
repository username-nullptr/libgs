
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

std::string wcstombs(const concepts::weak_wchar_string_type auto &str)
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	if constexpr( is_wchar_v<str_t> )
		return wcstombs(std::wstring_view(&str,1));
	else
	{
		std::wstring_view view(str);
		if( view.empty() )
			return "";

		auto size = view.size();
		auto buf = std::make_shared<char[]>(size + 1);

#ifdef _MSC_VER
		::wcstombs_s(&size, buf.get(), size, view.data(), size);
#else
		std::wcstombs(buf.get(), view.data(), size);
#endif
		return buf.get();
	}
}

auto mbstowcs(const concepts::weak_char_string_type auto &str)
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	if constexpr( is_char_v<str_t> )
	{
		auto wcs= mbstowcs(std::string_view(&str,1));
		return wcs[0];
	}
	else
	{
		std::string_view view(str);
		if( view.empty() )
			return std::wstring();

		auto size = view.size();
		auto buf = std::make_shared<wchar_t[]>(size + 1);

#ifdef _MSC_VER
		size_t rsize = 0;
		::mbstowcs_s(&rsize, buf.get(), size + 1, view.data(), size);
#else
		std::mbstowcs(buf.get(), view.data(), size);
#endif
		return std::wstring(buf.get());
	}
}

decltype(auto) xxtombs(concepts::weak_string_type auto &&str)
{
	using Str = decltype(str);
	using char_t = get_string_char_t<Str>;

	if constexpr( is_char_v<char_t> )
		return return_reference(std::forward<Str>(str));
	else
		return wcstombs(str);
}

decltype(auto) xxtowcs(concepts::weak_string_type auto &&str)
{
	using Str = decltype(str);
	using char_t = get_string_char_t<Str>;

	if constexpr( is_wchar_v<char_t> )
		return return_reference(std::forward<Str>(str));
	else
		return mbstowcs(str);
}

template <concepts::char_type CharT>
decltype(auto) mbstoxx(concepts::weak_char_string_type auto &&str)
{
	if constexpr( is_char_v<CharT> )
		return std::forward<decltype(str)>(str);
	else
		return mbstowcs(str);
}

template <concepts::char_type CharT>
decltype(auto) wcstoxx(concepts::weak_wchar_string_type auto &&str)
{
	if constexpr( is_wchar_v<CharT> )
		return std::forward<decltype(str)>(str);
	else
		return wcstombs(str);
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

bool is_alpha(const concepts::weak_string_type auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::char_type<Str> )
		return std::isalpha(str);
	else
	{
		using char_t = get_string_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isalpha(c);
		});
	}
}

bool is_digit(const concepts::weak_string_type auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::char_type<Str> )
		return std::isdigit(str);
	else
	{
		using char_t = get_string_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isdigit(c);
		});
	}
}

bool is_rlnum(const concepts::weak_string_type auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::char_type<Str> )
		return std::isdigit(str);
	else
	{
		using char_t = get_string_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		string_view_t view(str);
		if( view.empty() )
			return false;

		auto it = view.begin();
		if( *it == 0x2D/*-*/ or *it == 0x2B/*+*/ )
			++it;

		if( not std::isdigit(*it) )
			return false;

		bool dot = false;
		for(++it; it!=view.end(); ++it)
		{
			if( not std::isdigit(*it) )
			{
				if( *it == 0x2E/*.*/ and not dot )
					dot = true;
				else
					return false;
			}
		}
		return true;
	}
}

bool is_alnum(const concepts::weak_string_type auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::char_type<Str> )
		return std::isalnum(str);
	else
	{
		using char_t = get_string_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isalnum(c);
		});
	}
}

bool is_ascii(const concepts::weak_string_type auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::char_type<Str> )
		return str <= 0x7F;
	else
	{
		using char_t = get_string_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return c <= 0x7F;
		});
	}
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STRING_TOOLS_H