// SPDX-FileCopyrightText: 2024 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_MISC_H
#define LIBGS_CORE_ALGORITHM_DETAIL_MISC_H

namespace libgs { namespace detail
{

template <concepts::character CharT>
constexpr CharT to_hex_upper(unsigned int value) noexcept
{
	constexpr auto str = l_str(CharT,"0123456789ABCDEF");
	return str[value & 0xF];
}

template <concepts::character CharT>
constexpr CharT to_hex_lower(unsigned int value) noexcept
{
	constexpr auto str = l_str(CharT,"0123456789abcdef");
	return str[value & 0xF];
}

} //namespace detail

auto from_percent_encoding(concepts::any_string_p auto &&str, char percent)
{
	using Str = decltype(str);
	using char_t = strtls::get_char_t<Str>;

	std::basic_string<char_t> result(std::forward<Str>(str));
	if( result.empty() )
		return result;

	auto input_ptr = result.c_str();
	auto data = result.data();

	size_t i = 0;
	size_t outlen = 0;
	size_t len = result.size();

	int a = 0, b = 0;
	char_t c = 0;

	while( i < len )
	{
		c = input_ptr[i];
		if( c == static_cast<char_t>(percent) and i + 2 < len )
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

template <concepts::any_text_p Str, concepts::text_p<strtls::get_char_t<Str>> StrArg>
auto to_percent_encoding(const Str &str, StrArg &&exclude, StrArg &&include, char percent)
{
	using char_t = strtls::get_char_t<Str>;
	std::basic_string<char_t> result;

	decltype(auto) str_view = strtls::to_view(str);
	if( str_view.empty() )
		return result;

	decltype(auto) exclude_view = strtls::to_view(exclude);
	decltype(auto) include_view = strtls::to_view(include);

	const auto contains = [](std::basic_string_view<char_t> view, char_t c) {
		return not view.empty() and view.find(c) != std::basic_string_view<char_t>::npos;
	};
	size_t length = 0;
	result.resize(str_view.size());
	bool expanded = false;

	for(auto &c : str_view)
	{
		if( c != static_cast<char_t>(percent) and
			((c >= static_cast<char_t>(0x61) and c <= static_cast<char_t>(0x7A)) // ALPHA
			 or (c >= static_cast<char_t>(0x41) and c <= static_cast<char_t>(0x5A)) // ALPHA
			 or (c >= static_cast<char_t>(0x30) and c <= static_cast<char_t>(0x39)) // DIGIT
			 or c == static_cast<char_t>(0x2D) // -
			 or c == static_cast<char_t>(0x2E) // .
			 or c == static_cast<char_t>(0x5F) // _
			 or c == static_cast<char_t>(0x7E) // ~
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
			result[length++] = detail::to_hex_upper<char_t>((c & static_cast<char_t>(0xf0)) >> 4);
			result[length++] = detail::to_hex_upper<char_t>(c & static_cast<char_t>(0xf));
		}
	}
	if( expanded )
		result = result.substr(0, length);
	return result;
}

template <concepts::any_text_p Str, concepts::text_p<strtls::get_char_t<Str>> StrArg>
int32_t wildcard_match(const Str &rule, const StrArg &str)
{
	auto rule_view = strtls::to_view(rule);
	auto str_view = strtls::to_view(str);

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


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MISC_H
