
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

#include "misc.h"

namespace libgs { namespace detail
{

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> from_percent_encoding(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result;
	if( str.empty() )
		return result;

	result = str;
	auto *data = const_cast<CharT*>(result.c_str());
	const CharT *inputPtr = result.c_str();

	size_t i = 0;
	size_t len = str.size();
	size_t outlen = 0;
	int a, b;
	CharT c;

	while( i < len )
	{
		c = inputPtr[i];
		if( c == 0x25/*%*/ and i + 2 < len )
		{
			a = inputPtr[++i];
			b = inputPtr[++i];

			if( a >= 0x30/*0*/ and a <= 0x39/*9*/ )
				a -= 0x30/*0*/;

			else if( a >= 0x61/*a*/ and a <= 0x66/*f*/ )
				a = a - 0x61/*a*/ + 10;

			else if( a >= 0x41/*A*/ && a <= 0x46/*F*/ )
				a = a - 0x41/*A*/ + 10;

			if( b >= 0x30/*0*/ and b <= 0x39/*9*/ )
				b -= 0x30/*0*/;

			else if( b >= 0x61/*a*/ and b <= 0x66/*f*/ )
				b = b - 0x61/*a*/ + 10;

			else if( b >= 0x41/*A*/ and b <= 0x46/*F*/ )
				b = b - 0x41/*A*/ + 10;

			*data++ = static_cast<CharT>((a << 4) | b);
		}
		else
			*data++ = c;
		++i;
		++outlen;
	}
	if( outlen != len )
		result = result.substr(0, outlen);
	return result;
}

template <concepts::char_type CharT>
constexpr CharT to_hex_upper(unsigned int value) noexcept
{
	if constexpr( is_char_v<CharT> )
		return "0123456789ABCDEF"[value & 0xF];
	else
		return L"0123456789ABCDEF"[value & 0xF];
}

template <concepts::char_type CharT>
constexpr CharT to_hex_lower(unsigned int value) noexcept
{
	if constexpr( is_char_v<CharT> )
		return "0123456789abcdef"[value & 0xF];
	else
		return L"0123456789abcdef"[value & 0xF];
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> to_percent_encoding
(std::basic_string_view<CharT> str, std::basic_string_view<CharT> exclude, std::basic_string_view<CharT> include, char percent)
{
	if constexpr( is_wchar_v<CharT> )
		return mbstowcs(to_percent_encoding<char>(wcstombs(str), wcstombs(exclude), wcstombs(include), percent));
	else
	{
		std::string result;
		if( str.empty() )
			return result;

		const auto contains = [](std::string_view view, char c) {
			return not view.empty() and memchr(view.data(), c, view.size()) != nullptr;
		};
		size_t length = 0;
		result.resize(str.size());
		bool expanded = false;

		for(auto &c : str)
		{
			if( c != percent and
			    ((c >= 0x61 and c <= 0x7A) // ALPHA
			     or (c >= 0x41 and c <= 0x5A) // ALPHA
			     or (c >= 0x30 and c <= 0x39) // DIGIT
			     or c == 0x2D // -
			     or c == 0x2E // .
			     or c == 0x5F // _
			     or c == 0x7E // ~
			     or contains(exclude, c)) and
			    not contains(include, c) )
			{
				result[length++] = c;
			}
			else
			{
				if( not expanded )
				{
					result.resize(str.size() * 3);
					expanded = true;
				}
				result[length++] = percent;
				result[length++] = to_hex_upper<char>((c & 0xf0) >> 4);
				result[length++] = to_hex_upper<char>(c & 0xf);
			}
		}
		if( expanded )
			result = result.substr(0, length);
		return result;
	}
}

template <concepts::char_type CharT>
[[nodiscard]] static int32_t wildcard_match(std::basic_string_view<CharT> rule, std::basic_string_view<CharT> str)
{
	size_t rule_len = rule.size();
	size_t str_len = str.size();
	int32_t weight = 0;

	std::vector<std::vector<bool>> dp(str_len + 1, std::vector<bool>(rule_len + 1, false));
	dp[0][0] = true;

	for(size_t j=1; j<rule_len+1; j++)
	{
		if( rule[j-1] == 0x2A/***/ )
			dp[0][j] = dp[0][j-1];
	}
	for(size_t i=1; i<str_len+1; i++)
	{
		for(size_t j=1; j<rule_len+1; j++)
		{
			if( rule[j-1] == 0x3F/*?*/ )
			{
				dp[i][j] = dp[i-1][j-1];
				weight++;
			}
			else if( rule[j-1] == str[i-1] )
				dp[i][j] = dp[i-1][j-1];

			else if( rule[j-1] == 0x2A/***/ )
			{
				dp[i][j] = dp[i-1][j] or dp[i][j-1];
				weight += 2;
			}
		}
	}
	return dp.back().back() ? weight : -1;
}

} //namespace detail

std::string from_percent_encoding(std::string_view str)
{
	return detail::from_percent_encoding<char>(str);
}

std::wstring from_percent_encoding(std::wstring_view str)
{
	return detail::from_percent_encoding<wchar_t>(str);
}

std::string to_percent_encoding(std::string_view str, std::string_view exclude, std::string_view include, char percent)
{
	return detail::to_percent_encoding<char>(str, exclude, include, percent);
}

std::wstring to_percent_encoding(std::wstring_view str, std::wstring_view exclude, std::wstring_view include, char percent)
{
	return detail::to_percent_encoding<wchar_t>(str, exclude, include, percent);
}

int32_t wildcard_match(std::string_view rule, std::string_view str)
{
	return detail::wildcard_match<char>(rule, str);
}

int32_t wildcard_match(std::wstring_view rule, std::wstring_view str)
{
	return detail::wildcard_match<wchar_t>(rule, str);
}

} //namespace libgs