
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

#ifndef LIBGS_CORE_UTILS_DETAIL_STRING_TOOLS_H
#define LIBGS_CORE_UTILS_DETAIL_STRING_TOOLS_H

namespace libgs::strtls { namespace detail
{

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> ascii_transition(const concepts::any_text auto &text)
{
	using Text = decltype(text);
	using str_t = std::remove_cvref_t<Text>;

	if constexpr( is_any_char_v<str_t> )
		return std::basic_string<CharT>(&text,1);

	else if constexpr( std::is_same_v<get_char_t<Text>, CharT> )
		return std::forward<Text>(text);
	else
	{
		decltype(auto) view = to_view(std::forward<Text>(text));
		std::basic_string<CharT> res(view.size(), '\0');

		for(size_t i=0; i<view.size(); i++)
			res[i] = static_cast<CharT>(view[i]);
		return res;
	}
}

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI auto _sto_float(auto &&func, std::basic_string_view<CharT> str)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index);
	if( index < str.size() )
		throw std::runtime_error("Cannot convert string to arithmetic.");
	return res;
}

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI auto _sto_int(auto &&func, std::basic_string_view<CharT> str, size_t base)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index, static_cast<int>(base));
	if( index < str.size() )
	{
		res = static_cast<decltype(res)>(_sto_float<CharT>(
			static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str
		));
	}
	return res;
}

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI int _to_bool(std::basic_string_view<CharT> str)
{
	constexpr auto true_text = l_str(CharT, "true");
	constexpr auto false_text = l_str(CharT, "false");

#ifdef WIN32
	if( _stricmp(str.data(), true_text) == 0 )
		return 1;
	else if( _stricmp(str.data(), false_text) == 0 )
		return 0;
#else
	if( str.size() == 4 and strncasecmp(str.data(), true_text, 4) == 0 )
		return 1;
	else if( str.size() == 5 and strncasecmp(str.data(), false_text, 5) == 0 )
		return 0;
#endif
	return -1;
}

template <concepts::character CharT, typename T>
[[nodiscard]] LIBGS_CORE_TAPI T try_to_booltot(std::basic_string_view<CharT> str, const std::optional<T> &odv)
{
	int res = _to_bool<CharT>(str);
	if( res < 0 )
	{
		if( odv )
			return *odv;
		throw std::runtime_error("Cannot convert string to arithmetic.");
	}
	return static_cast<T>(!!res);
}

[[nodiscard]] LIBGS_CORE_TAPI int8_t to_int8(const auto &str, size_t base, std::optional<int8_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<int8_t>(_sto_int<char_t>(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI uint8_t to_uint8(const auto &str, size_t base, std::optional<uint8_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<uint8_t>(_sto_int<char_t>(
			static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI int16_t to_int16(const auto &str, size_t base, std::optional<int16_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<int16_t>(_sto_int<char_t>(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI uint16_t to_uint16(const auto &str, size_t base, std::optional<uint8_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<uint16_t>(_sto_int<char_t>(
			static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI int32_t to_int32(const auto &str, size_t base, std::optional<int32_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<int32_t>(_sto_int<char_t>(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI uint32_t to_uint32(const auto &str, size_t base, std::optional<uint32_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<uint32_t>(_sto_int<char_t>(
			static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI int64_t to_int64(const auto &str, size_t base, std::optional<int64_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<int64_t>(_sto_int<char_t>(
			static_cast<long long(*)(const string_t&,size_t*,int)>(std::stoll), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI uint64_t to_uint64(const auto &str, size_t base, std::optional<uint64_t> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return static_cast<uint64_t>(_sto_int<char_t>(
			static_cast<unsigned long long(*)(const string_t&,size_t*,int)>(std::stoull), view, base
		));
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI float to_float(const auto &str, std::optional<float> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return _sto_float<char_t>(
			static_cast<float(*)(const string_t&,size_t*)>(std::stof), view
		);
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI double to_double(const auto &str, std::optional<double> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return _sto_float<char_t>(
			static_cast<double(*)(const string_t&,size_t*)>(std::stod), view
		);
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI long double to_ldouble(const auto &str, std::optional<long double> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		return _sto_float<char_t>(
			static_cast<long double(*)(const string_t&,size_t*)>(std::stold), view
		);
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);

}

[[nodiscard]] LIBGS_CORE_TAPI bool to_bool(const auto &str, size_t base, std::optional<bool> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	if constexpr( concepts::character<str_t> )
		return str != 0x30;
	else
	{
		using char_t = get_char_t<str_t>;
		std::basic_string_view<char_t> view(str);

		int res = _to_bool<char_t>(view);
		if( res < 0 )
		{
			using char_t = get_char_t<str_t>;
			using string_t = std::basic_string<char_t>;
			try {
				return /*!!*/_sto_int<char_t>(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
				);
			}
			catch(...)
			{
				if( odv.has_value() )
					return *odv;
				throw std::runtime_error("Cannot convert string to arithmetic.");
			}
		}
		return res > 0;
	}
}

template <concepts::integral_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith(const auto &str, size_t base, std::optional<T> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	auto view = to_view(str);

	if constexpr( std::is_same_v<T, bool> )
		return to_bool<char_t>(view, base);
	else
	{
		using string_t = std::basic_string<char_t>;
		try {
			if constexpr( std::is_same_v<T, char> )
			{
				return static_cast<char>(_sto_int<char_t>(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned char> )
			{
				return static_cast<unsigned char>(_sto_int<char_t>(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
				));
			}
			else if constexpr( std::is_same_v<T, short> )
			{
				return static_cast<short>(_sto_int<char_t>(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned short> )
			{
				return static_cast<unsigned short>(_sto_int<char_t>(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
				));
			}
			else if constexpr( std::is_same_v<T, int> )
			{
				return static_cast<int>(_sto_int<char_t>(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned int> )
			{
				return static_cast<unsigned int>(_sto_int<char_t>(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
				));
			}
			else if constexpr( std::is_same_v<T, long> )
			{
				return static_cast<long>(_sto_int<char_t>(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), view, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long> )
			{
				return static_cast<unsigned long>(_sto_int<char_t>(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), view, base
				));
			}
			else if constexpr( std::is_same_v<T, long long> )
			{
				return static_cast<long long>(_sto_int<char_t>(
					static_cast<long long(*)(const string_t&,size_t*,int)>(std::stoll), view, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long long> )
			{
				return static_cast<unsigned long long>(_sto_int<char_t>(
					static_cast<unsigned long long(*)(const string_t&,size_t*,int)>(std::stoull), view, base
				));
			}
		}
		catch(std::exception&) {}
		return try_to_booltot<char_t>(view, odv);
	}
}

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith(const auto &str, std::optional<T> odv = {})
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;
	using string_t = std::basic_string<char_t>;

	auto view = to_view(str);
	try {
		if constexpr( std::is_same_v<T, float> )
		{
			return _sto_float<char_t>(
				static_cast<float(*)(const string_t&,size_t*)>(std::stof), view
			);
		}
		else if constexpr( std::is_same_v<T, double> )
		{
			return _sto_float<char_t>(
				static_cast<double(*)(const string_t&,size_t*)>(std::stod), view
			);
		}
		else if constexpr( std::is_same_v<T, long double> )
		{
			return _sto_float<char_t>(
				static_cast<long double(*)(const string_t&,size_t*)>(std::stold), view
			);
		}
	}
	catch(std::exception&) {}
	return try_to_booltot<char_t>(view, odv);
}

template <concepts::character CharT>
LIBGS_CORE_TAPI size_t replace
(std::basic_string<CharT> &str, std::basic_string_view<CharT> find, std::basic_string_view<CharT> repl, bool step)
{
	if( find == repl )
		return 0;

	size_t sum = 0;
	size_t find_pos = 0;
	for(;;)
	{
		auto start = str.find(find, find_pos);
		if( start == std::basic_string<CharT>::npos )
			break;

		str.replace(start, find.size(), repl);
		find_pos = start;

		if( step )
			find_pos += repl.size();
		sum++;
	}
	return sum;
}

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI auto find_separator(std::basic_string_view<CharT> view)
{
	size_t pos = view.rfind(static_cast<CharT>('/'));
	return pos == std::basic_string<CharT>::npos ?
		view.rfind(static_cast<CharT>('\\')) : pos;
}

} //namespace detail

decltype(auto) to_string(concepts::any_text_p auto &&text)
{
	using Text = decltype(text);
	using char_t = get_char_t<Text>;

	if constexpr( is_any_std_string_v<Text> )
		return return_reference(std::forward<Text>(text));
	else if constexpr( is_any_char_v<std::remove_cvref_t<Text>> )
		return std::basic_string<char_t>(&text,1);
	else
		return std::basic_string<char_t>(std::forward<Text>(text));
}

decltype(auto) to_view(concepts::any_text_p auto &&text)
{
	using Text = decltype(text);
	using char_t = get_char_t<Text>;

	if constexpr( is_any_char_v<std::remove_cvref_t<Text>> )
		return std::basic_string_view<char_t>(&text,1);

	else if constexpr( is_any_std_string_view_v<Text> )
		return return_reference(std::forward<Text>(text));
	else
		return std::basic_string_view<char_t>(std::forward<Text>(text));
}

bool is_alpha(const concepts::any_string_p auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::character<Str> )
		return std::isalpha(str);
	else
	{
		using char_t = get_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isalpha(c);
		});
	}
}

bool is_digit(const concepts::any_string_p auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::character<Str> )
		return std::isdigit(str);
	else
	{
		using char_t = get_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isdigit(c);
		});
	}
}

bool is_rlnum(const concepts::any_string_p auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::character<Str> )
		return std::isdigit(str);
	else
	{
		using char_t = get_char_t<Str>;
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

bool is_alnum(const concepts::any_string_p auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::character<Str> )
		return std::isalnum(str);
	else
	{
		using char_t = get_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return std::isalnum(c);
		});
	}
}

bool is_ascii(const concepts::any_string_p auto &str) noexcept
{
	using Str = decltype(str);
	if constexpr( concepts::character<Str> )
		return str <= 0x7F;
	else
	{
		using char_t = get_char_t<Str>;
		using string_view_t = std::basic_string_view<char_t>;

		return std::ranges::all_of(string_view_t(str), [](auto c){
			return c <= 0x7F;
		});
	}
}

int8_t to_int8(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_int8(text, base);
}

uint8_t to_uint8(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_uint8(text, base);
}

int16_t to_int16(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_int16(text, base);
}

uint16_t to_uint16(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_uint16(text, base);
}

int32_t to_int32(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_int32(text, base);
}

uint32_t to_uint32(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_uint32(text, base);
}

int64_t to_int64(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_int64(text, base);
}

uint64_t to_uint64(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_uint64(text, base);
}

float to_float(const concepts::any_text_p auto &text)
{
	return detail::to_float(text);
}

double to_double(const concepts::any_text_p auto &text)
{
	return detail::to_double(text);
}

long double to_ldouble(const concepts::any_text_p auto &text)
{
	return detail::to_ldouble(text);
}

bool to_bool(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_bool(text, base);
}

template <concepts::integral_p T>
[[nodiscard]] T to_arith(const concepts::any_text_p auto &text, size_t base)
{
	return detail::to_arith<T>(text, base);
}

template <concepts::floating_p T>
[[nodiscard]] T to_arith(const concepts::any_text_p auto &text)
{
	return detail::to_arith<T>(text);
}

int8_t to_int8_or(const concepts::any_text_p auto &text, int8_t default_value, size_t base) noexcept
{
	return detail::to_int8(text, base, default_value);
}

uint8_t to_uint8_or(const concepts::any_text_p auto &text, uint8_t default_value, size_t base) noexcept
{
	return detail::to_int8(text, base, default_value);
}

int16_t to_int16_or(const concepts::any_text_p auto &text, int16_t default_value, size_t base) noexcept
{
	return detail::to_int16(text, base, default_value);
}

uint16_t to_uint16_or(const concepts::any_text_p auto &text, uint16_t default_value, size_t base) noexcept
{
	return detail::to_uint16(text, base, default_value);
}

int32_t to_int32_or(const concepts::any_text_p auto &text, int32_t default_value, size_t base) noexcept
{
	return detail::to_int32(text, base, default_value);
}

uint32_t to_uint32_or(const concepts::any_text_p auto &text, uint32_t default_value, size_t base) noexcept
{
	return detail::to_uint32(text, base, default_value);
}

int64_t to_int64_or(const concepts::any_text_p auto &text, int64_t default_value, size_t base) noexcept
{
	return detail::to_int64(text, base, default_value);
}

uint64_t to_uint64_or(const concepts::any_text_p auto &text, uint64_t default_value, size_t base) noexcept
{
	return detail::to_uint64(text, base, default_value);
}

float to_float_or(const concepts::any_text_p auto &text, float default_value) noexcept
{
	return detail::to_float(text, default_value);
}

double to_double_or(const concepts::any_text_p auto &text, double default_value) noexcept
{
	return detail::to_double(text, default_value);
}

long double to_ldouble_or(const concepts::any_text_p auto &text, long double default_value) noexcept
{
	return detail::to_ldouble(text, default_value);
}

bool to_bool_or(const concepts::any_text_p auto &text, bool default_value, size_t base) noexcept
{
	return detail::to_bool(text, base, default_value);
}

template <concepts::integral_p T>
[[nodiscard]] T to_arith_or(const concepts::any_text_p auto &text, T default_value, size_t base) noexcept
{
	return detail::to_arith<T>(text, base, default_value);
}

template <concepts::floating_p T>
[[nodiscard]] T to_arith_or(const concepts::any_text_p auto &text, T default_value) noexcept
{
	return detail::to_arith<T>(text, default_value);
}

auto to_lower(concepts::any_text_p auto &&text)
{
	using Text = decltype(text);
	using text_t = std::remove_cvref_t<Text>;
	using char_t = get_char_t<text_t>;

	if constexpr( concepts::character<text_t> )
		return static_cast<char_t>(std::tolower(text));
	else
	{
		std::basic_string<char_t> result(std::forward<Text>(text));
		for(auto &c : result)
			c = static_cast<char_t>(std::tolower(c));
		return result;
	}
}

auto to_upper(concepts::any_text_p auto &&text)
{
	using Text = decltype(text);
	using text_t = std::remove_cvref_t<Text>;
	using char_t = get_char_t<text_t>;

	if constexpr( concepts::character<text_t> )
		return static_cast<char_t>(std::toupper(text));
	else
	{
		std::basic_string<char_t> result(std::forward<Text>(text));
		for(auto &c : result)
			c = static_cast<char_t>(std::toupper(c));
		return result;
	}
}

template <concepts::any_string_p Str>
auto replace(Str &&str, concepts::text_p<get_char_t<Str>> auto &&find,
			 concepts::text_p<get_char_t<Str>> auto &&repl, bool step)
{
	std::basic_string<get_char_t<Str>> result(std::forward<Str>(str));
	detail::replace(result,
		to_view(std::forward<decltype(find)>(find)),
		to_view(std::forward<decltype(repl)>(repl)),
		step
	);
	return result;
}

template <concepts::any_string_p Str>
auto replace(size_t &count, Str &&str, concepts::text_p<get_char_t<Str>> auto &&find,
			 concepts::text_p<get_char_t<Str>> auto &&repl, bool step)
{
	std::basic_string<get_char_t<Str>> result(std::forward<Str>(str));
	count = detail::replace(result,
		to_view(std::forward<decltype(find)>(find)),
		to_view(std::forward<decltype(repl)>(repl)),
		step
	);
	return result;
}

auto trimmed(const concepts::any_text_p auto &text)
{
	using text_t = std::remove_cvref_t<decltype(text)>;
	using char_t = get_char_t<text_t>;

	if constexpr( concepts::character<text_t> )
		return trimmed(std::basic_string_view(&text,1));
	else
	{
		decltype(auto) view = to_view(text);
		std::basic_string<char_t> result;

		size_t left = 0;
		while( left < view.size() )
		{
			if( view[left] >= 1 and view[left] <= 32 )
				left++;
			else
				break;
		}
		if( left >= view.size() )
			return result;

		int right = static_cast<int>(view.size() - 1);
		while( right >= static_cast<int>(left) )
		{
			if( view[right] >= 1 and view[right] <= 32 )
				--right;
			else
				break;
		}
		if( right < static_cast<int>(left) )
			return result;

		result = view.substr(0, right + 1UL);
		result = result.substr(left);
		return result;
	}
}

template <concepts::any_string_p Str, concepts::text_p<get_char_t<Str>> Find>
auto remove(const Str &str, const Find &find, bool step)
{
	using str_t = std::remove_cvref_t<decltype(str)>;
	using char_t = get_char_t<str_t>;

	if constexpr( concepts::character<str_t> )
	{
		std::basic_string<char_t> res(str.data(), str.size());
		auto it = std::remove(res.begin(), res.end(), find);
		if( it != res.end() )
			res.erase(it, res.end());
		return res;
	}
	else
	{
		std::basic_string<char_t> res(str.data(), str.size());
		replace(res, find, std::basic_string<char_t>(), step);
		return res;
	}
}

auto file_name(const concepts::any_text_p auto &file_name)
{
	using text_t = std::remove_cvref_t<decltype(file_name)>;
	using char_t = get_char_t<text_t>;

	using str_t = std::basic_string<char_t>;
	using str_view_t = std::basic_string_view<char_t>;

	if constexpr( concepts::character<text_t> )
		return str_t(&file_name,1);
	else
	{
		decltype(auto) view = to_view(file_name);
		auto pos = detail::find_separator(view);

		if( pos == str_view_t::npos )
			return str_t(view);

		auto tmp = view.substr(pos + 1);
		return str_t(tmp);
	}
}

auto file_path(const concepts::any_text_p auto &file_name)
{
	using text_t = std::remove_cvref_t<decltype(file_name)>;
	using char_t = get_char_t<text_t>;

	using str_t = std::basic_string<char_t>;
	using str_view_t = std::basic_string_view<char_t>;

	if constexpr( concepts::character<text_t> )
		return strtls::file_path(str_view_t(&file_name,1));
	else
	{
		decltype(auto) view = to_view(file_name);
		auto pos = detail::find_separator(view);

		if( pos == str_view_t::npos )
			return str_t(l_str(char_t,"./"));

		auto tmp = view.substr(0, pos + 1);
		return str_t(tmp);
	}
}

} //namespace libgs::strtls


#endif //LIBGS_CORE_UTILS_DETAIL_STRING_TOOLS_H