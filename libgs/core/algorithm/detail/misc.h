
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

namespace libgs { namespace detail
{

template <concepts::character CharT>
constexpr CharT to_hex_upper(unsigned int value) noexcept
{
	return s_str<CharT,
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
	>[value & 0xF];
}

template <concepts::character CharT>
constexpr CharT to_hex_lower(unsigned int value) noexcept
{
	return s_str<CharT,
		'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
	>[value & 0xF];
}

} //namespace detail

auto from_percent_encoding(concepts::string_type auto &&str)
{
	using Str = decltype(str);
	using char_t = get_string_char_t<Str>;

	std::basic_string<char_t> result(std::forward<Str>(str));
	if( result.empty() )
		return result;

	auto input_ptr = result.c_str();
	auto data = result.data();

	size_t i = 0;
	size_t len = str.size();
	size_t outlen = 0;

	int a = 0, b = 0;
	char_t c = 0;

	while( i < len )
	{
		c = input_ptr[i];
		if( c == 0x25/*%*/ and i + 2 < len )
		{
			a = input_ptr[++i];
			b = input_ptr[++i];

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

			*data++ = static_cast<char_t>((a << 4) | b);
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

template <concepts::weak_string_type Str, concepts::weak_basic_string_type<get_string_char_t<Str>> StrArg>
std::string to_percent_encoding(const Str &str, StrArg &&exclude, StrArg &&include, char percent)
{
	using char_t = get_string_char_t<Str>;
	if constexpr( not is_char_v<char_t> )
	{
		std::basic_string_view<char_t> view(str);
		constexpr auto char_len = sizeof(char_t);

		std::string char_str(view.size() * char_len, '\0');
		for(size_t i=0; i<view.size(); i++)
		{
			auto &wc = view[i];
			for(size_t j=0; j<char_len; j++)
				char_str[i * char_len + j] = wc >> (8 * (char_len - 1 - j));
		}
		return to_percent_encoding(std::move(char_str),
			std::forward<StrArg>(exclude), std::forward<StrArg>(include), percent
		);
	}
	else
	{
		auto str_view = transition_string_view(str);
		auto exclude_view = transition_string_view(exclude);
		auto include_view = transition_string_view(include);

		std::string result;
		if( str_view.empty() )
			return result;

		const auto contains = [](std::string_view view, char c) {
			return not view.empty() and memchr(view.data(), c, view.size()) != nullptr;
		};
		size_t length = 0;
		result.resize(str_view.size());
		bool expanded = false;

		for(auto &c : str_view)
		{
			if( c != percent and
			    ((c >= 0x61 and c <= 0x7A) // ALPHA
			     or (c >= 0x41 and c <= 0x5A) // ALPHA
			     or (c >= 0x30 and c <= 0x39) // DIGIT
			     or c == 0x2D // -
			     or c == 0x2E // .
			     or c == 0x5F // _
			     or c == 0x7E // ~
			     or contains(exclude_view, c)) and
			    not contains(include_view, c) )
			{
				result[length++] = c;
			}
			else
			{
				if( not expanded )
				{
					result.resize(str_view.size() * 3);
					expanded = true;
				}
				result[length++] = percent;
				result[length++] = detail::to_hex_upper<char>((c & 0xf0) >> 4);
				result[length++] = detail::to_hex_upper<char>(c & 0xf);
			}
		}
		if( expanded )
			result = result.substr(0, length);
		return result;
	}
}

template <concepts::weak_string_type Str, concepts::weak_basic_string_type<get_string_char_t<Str>> StrArg>
int32_t wildcard_match(const Str &rule, const StrArg &str)
{
	auto rule_view = transition_string_view(rule);
	auto str_view = transition_string_view(str);

	size_t rule_len = rule_view.size();
	size_t str_len = str_view.size();
	int32_t weight = 0;

	std::vector dp(str_len + 1, std::vector(rule_len + 1, false));
	dp[0][0] = true;

	for(size_t j=1; j<rule_len+1; j++)
	{
		if( rule_view[j-1] == 0x2A/***/ )
			dp[0][j] = dp[0][j-1];
	}
	for(size_t i=1; i<str_len+1; i++)
	{
		for(size_t j=1; j<rule_len+1; j++)
		{
			if( rule_view[j-1] == 0x3F/*?*/ )
			{
				dp[i][j] = dp[i-1][j-1];
				weight++;
			}
			else if( rule_view[j-1] == str_view[i-1] )
				dp[i][j] = dp[i-1][j-1];

			else if( rule_view[j-1] == 0x2A/***/ )
			{
				dp[i][j] = dp[i-1][j] or dp[i][j-1];
				weight += 2;
			}
		}
	}
	return dp.back().back() ? weight : -1;
}

} //namespace libgs