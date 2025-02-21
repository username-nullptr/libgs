
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_BASE_H
#define LIBGS_CORE_ALGORITHM_DETAIL_BASE_H

namespace libgs { namespace detail
{

[[nodiscard]] LIBGS_CORE_TAPI auto _sto_float(auto &&func, const auto &str)
{
	using char_t = get_string_char_t<decltype(str)>;
	std::basic_string_view<char_t> view(str);

	size_t index = 0;
	auto res = func({view.data(), view.size()}, &index);
	if( index < view.size() )
		throw runtime_error("Cannot convert string to arithmetic.");
	return res;
}

[[nodiscard]] LIBGS_CORE_TAPI auto _sto_int(auto &&func, const auto &str, size_t base)
{
	using char_t = get_string_char_t<decltype(str)>;
	std::basic_string_view<char_t> view(str);

	size_t index = 0;
	auto res = func({view.data(), view.size()}, &index, static_cast<int>(base));
	if( index < view.size() )
	{
		res = static_cast<decltype(res)>(
			_sto_float(static_cast<long double(*)(const std::basic_string<char_t>&,size_t*)>(std::stold), str)
		);
	}
	return res;
}

template <concepts::char_type>
struct bool_string;

template <>
struct bool_string<char>
{
	static constexpr auto true_text = "true";
	static constexpr auto false_text = "false";
};

template <>
struct bool_string<wchar_t>
{
	static constexpr auto true_text = L"true";
	static constexpr auto false_text = L"false";
};

[[nodiscard]] LIBGS_CORE_TAPI int _stob(const auto &str)
{
	using char_t = get_string_char_t<decltype(str)>;
	std::basic_string_view<char_t> view(str);

	constexpr auto true_text = bool_string<char_t>::true_text;
	constexpr auto false_text = bool_string<char_t>::false_text;

#ifdef WIN32
	if( _stricmp(view.data(), true_text) == 0 )
		return 1;
	else if( _stricmp(view.data(), false_text) == 0 )
		return 0;
#else
	if( view.size() == 4 and strncasecmp(view.data(), true_text, 4) == 0 )
		return 1;
	else if( view.size() == 5 and strncasecmp(view.data(), false_text, 5) == 0 )
		return 0;
#endif
	return -1;
}

template <typename T>
[[nodiscard]] LIBGS_CORE_TAPI T try_stobtot(const auto &str, const std::optional<T> &odv)
{
	int res = _stob(str);
	if( res < 0 )
	{
		if( odv )
			return *odv;
		throw runtime_error("Cannot convert string to arithmetic.");
	}
	return static_cast<T>(!!res);
}

[[nodiscard]] LIBGS_CORE_TAPI int16_t stoi8(const auto &str, size_t base, std::optional<int8_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<int8_t>(_sto_int(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI uint16_t stou8(const auto &str, size_t base, std::optional<uint8_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<uint8_t>(_sto_int(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI int16_t stoi16(const auto &str, size_t base, std::optional<int16_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<int16_t>(_sto_int(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI uint16_t stou16(const auto &str, size_t base, std::optional<uint8_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<uint16_t>(_sto_int(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI int32_t stoi32(const auto &str, size_t base, std::optional<int32_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<int32_t>(_sto_int(
			static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI uint32_t stou32(const auto &str, size_t base, std::optional<uint32_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<uint32_t>(_sto_int(
			static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI int64_t stoi64(const auto &str, size_t base, std::optional<int64_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<int64_t>(_sto_int(
			static_cast<long long(*)(const string_t&,size_t*,int)>(std::stoll), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI uint64_t stou64(const auto &str, size_t base, std::optional<uint64_t> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return static_cast<uint64_t>(_sto_int(
			static_cast<unsigned long long(*)(const string_t&,size_t*,int)>(std::stoull), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI float stof(const auto &str, std::optional<float> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return _sto_float(
			static_cast<float(*)(const string_t&,size_t*)>(std::stof), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI double stod(const auto &str, std::optional<double> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return _sto_float(
			static_cast<double(*)(const string_t&,size_t*)>(std::stod), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI long double stold(const auto &str, std::optional<long double> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		return _sto_float(
			static_cast<long double(*)(const string_t&,size_t*)>(std::stold), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

[[nodiscard]] LIBGS_CORE_TAPI bool stob(const auto &str, size_t base, std::optional<bool> odv = {})
{
	int res = _stob(str);
	if( res < 0 )
	{
		using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
		try {
			return /*!!*/_sto_int(
				static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
			);
		}
		catch(...)
		{
			if( odv.has_value() )
				return *odv;
			throw runtime_error("Cannot convert string to arithmetic.");
		}
	}
	return res > 0;
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(const auto &str, size_t base, std::optional<T> odv = {})
{
	if constexpr( std::is_same_v<T, bool> )
		return stob(str, base);
	else
	{
		using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
		try {
			if constexpr( std::is_same_v<T, char> )
			{
				return static_cast<char>(_sto_int(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned char> )
			{
				return static_cast<unsigned char>(_sto_int(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, short> )
			{
				return static_cast<short>(_sto_int(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned short> )
			{
				return static_cast<unsigned short>(_sto_int(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, int> )
			{
				return static_cast<int>(_sto_int(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned int> )
			{
				return static_cast<unsigned int>(_sto_int(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, long> )
			{
				return static_cast<long>(_sto_int(
					static_cast<long(*)(const string_t&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long> )
			{
				return static_cast<unsigned long>(_sto_int(
					static_cast<unsigned long(*)(const string_t&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, long long> )
			{
				return static_cast<long long>(_sto_int(
					static_cast<long long(*)(const string_t&,size_t*,int)>(std::stoll), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long long> )
			{
				return static_cast<unsigned long long>(_sto_int(
					static_cast<unsigned long long(*)(const string_t&,size_t*,int)>(std::stoull), str, base
				));
			}
		}
		catch(std::exception&) {}
		return try_stobtot(str, odv);
	}
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(const auto &str, std::optional<T> odv = {})
{
	using string_t = std::basic_string<get_string_char_t<decltype(str)>>;
	try {
		if constexpr( std::is_same_v<T, float> )
		{
			return _sto_float(
				static_cast<float(*)(const string_t&,size_t*)>(std::stof), str
			);
		}
		else if constexpr( std::is_same_v<T, double> )
		{
			return _sto_float(
				static_cast<double(*)(const string_t&,size_t*)>(std::stod), str
			);
		}
		else if constexpr( std::is_same_v<T, long double> )
		{
			return _sto_float(
				static_cast<long double(*)(const string_t&,size_t*)>(std::stold), str
			);
		}
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <typename Str>
[[nodiscard]] LIBGS_CORE_TAPI auto str_to_lower(Str &&str)
{
	using char_t = get_string_char_t<Str>;
	std::basic_string<char_t> result(std::forward<Str>(str));

	for(auto &c : result)
		c = static_cast<char_t>(std::tolower(c));
	return result;
}

template <typename Str>
[[nodiscard]] LIBGS_CORE_TAPI auto str_to_upper(Str &&str)
{
	using char_t = get_string_char_t<Str>;
	std::basic_string<char_t> result(std::forward<Str>(str));

	for(auto &c : result)
		c = static_cast<char_t>(std::toupper(c));
	return result;
}

template <concepts::char_type CharT>
LIBGS_CORE_TAPI size_t str_replace(std::basic_string<CharT> &str, const str_replace_condition<CharT> &cond)
{
	if( cond.find == cond.repl )
		return 0;

	size_t sum = 0;
	size_t find_pos = 0;
	for(;;)
	{
		auto start = str.find(cond.find, find_pos);
		if( start == std::basic_string<CharT>::npos )
			break;

		str.replace(start, cond.find.size(), cond.repl);
		find_pos = start;

		if( cond.step )
			find_pos += cond.repl.size();
		sum++;
	}
	return sum;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> str_trimmed(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result;

	size_t left = 0;
	while( left < str.size() )
	{
		if( str[left] >= 1 and str[left] <= 32 )
			left++;
		else
			break;
	}
	if( left >= str.size() )
		return result;

	int right = static_cast<int>(str.size() - 1);
	while( right >= left )
	{
		if( str[right] >= 1 and str[right] <= 32 )
			right--;
		else
			break;
	}
	if( right < left )
		return result;

	result = str.substr(0, right + 1);
	result = result.substr(left);
	return result;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT>
str_remove(std::basic_string_view<CharT> str, std::basic_string_view<CharT> find, bool step)
{
	std::basic_string<CharT> res(str.data(), str.size());
	str_replace<CharT>(res, find, std::basic_string<CharT>(), step);
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT>
str_remove(std::basic_string_view<CharT> str, CharT find, bool)
{
	std::basic_string<CharT> res(str.data(), str.size());
	auto it = std::remove(res.begin(), res.end(), find);
	if( it != res.end() )
		res.erase(it, res.end());
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> file_name(std::basic_string_view<CharT> file_name)
{
	size_t pos = 0;
	if constexpr( is_char_v<CharT> )
		pos = file_name.rfind("/");
	else
		pos = file_name.rfind(L"/");

	if( pos == std::basic_string<CharT>::npos )
	{
		if constexpr( is_char_v<CharT> )
			pos = file_name.rfind("\\");
		else
			pos = file_name.rfind(L"\\");

		if( pos == std::basic_string<CharT>::npos )
			return std::basic_string<CharT>(file_name.data(), file_name.size());
	}
	auto tmp = file_name.substr(pos + 1);
	return std::basic_string<CharT>(tmp.data(), tmp.size());
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> file_path(std::basic_string_view<CharT> file_name)
{
	size_t pos = 0;
	if constexpr( is_char_v<CharT> )
	{
		pos = file_name.rfind("/");
		if( pos == std::basic_string<CharT>::npos )
			return ".";
	}
	else
	{
		pos = file_name.rfind(L"/");
		if( pos == std::basic_string<CharT>::npos )
			return L".";
	}
	auto tmp = file_name.substr(0, pos + 1);
	return std::basic_string<CharT>(tmp.data(), tmp.size());
}

} //namespace detail

int8_t stoi8(const concepts::string_type auto &str, size_t base)
{
	return detail::stoi8(str, base);
}

uint8_t stou8(const concepts::string_type auto &str, size_t base)
{
	return detail::stou8(str, base);
}

int16_t stoi16(const concepts::string_type auto &str, size_t base)
{
	return detail::stoi16(str, base);
}

uint16_t stou16(const concepts::string_type auto &str, size_t base)
{
	return detail::stou16(str, base);
}

int32_t stoi32(const concepts::string_type auto &str, size_t base)
{
	return detail::stoi32(str, base);
}

uint32_t stou32(const concepts::string_type auto &str, size_t base)
{
	return detail::stou32(str, base);
}

int64_t stoi64(const concepts::string_type auto &str, size_t base)
{
	return detail::stoi64(str, base);
}

uint64_t stou64(const concepts::string_type auto &str, size_t base)
{
	return detail::stou64(str, base);
}

float stof(const concepts::string_type auto &str)
{
	return detail::stof(str);
}

double stod(const concepts::string_type auto &str)
{
	return detail::stod(str);
}

long double stold(const concepts::string_type auto &str)
{
	return detail::stold(str);
}

bool stob(const concepts::string_type auto &str, size_t base)
{
	return detail::stob(str, base);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(const concepts::string_type auto &str, size_t base)
{
	return detail::ston<T>(str, base);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(const concepts::string_type auto &str)
{
	return detail::ston<T>(str);
}

int8_t stoi8_or(const concepts::string_type auto &str, size_t base, int8_t default_value) noexcept
{
	return detail::stoi8(str, base, default_value);
}

uint8_t stou8_or(const concepts::string_type auto &str, size_t base, uint8_t default_value) noexcept
{
	return detail::stoi8(str, base, default_value);
}

int16_t stoi16_or(const concepts::string_type auto &str, size_t base, int16_t default_value) noexcept
{
	return detail::stoi16(str, base, default_value);
}

uint16_t stou16_or(const concepts::string_type auto &str, size_t base, uint16_t default_value) noexcept
{
	return detail::stou16(str, base, default_value);
}

int32_t stoi32_or(const concepts::string_type auto &str, size_t base, int32_t default_value) noexcept
{
	return detail::stoi32(str, base, default_value);
}

uint32_t stou32_or(const concepts::string_type auto &str, size_t base, uint32_t default_value) noexcept
{
	return detail::stou32(str, base, default_value);
}

int64_t stoi64_or(const concepts::string_type auto &str, size_t base, int64_t default_value) noexcept
{
	return detail::stoi64(str, base, default_value);
}

uint64_t stou64_or(const concepts::string_type auto &str, size_t base, uint64_t default_value) noexcept
{
	return detail::stou64(str, base, default_value);
}

float stof_or(const concepts::string_type auto &str, float default_value) noexcept
{
	return detail::stof(str, default_value);
}

double stod_or(const concepts::string_type auto &str, double default_value) noexcept
{
	return detail::stod(str, default_value);
}

long double stold_or(const concepts::string_type auto &str, long double default_value) noexcept
{
	return detail::stold(str, default_value);
}

bool stob_or(const concepts::string_type auto &str, size_t base, bool default_value) noexcept
{
	return detail::stob(str, base, default_value);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(const concepts::string_type auto &str, size_t base, T default_value) noexcept
{
	return detail::ston<T>(str, base, default_value);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(const concepts::string_type auto &str, T default_value)
{
	return detail::ston<T>(str, default_value);
}

auto str_to_lower(concepts::string_type auto &&str)
{
	using string_t = decltype(str);
	return detail::str_to_lower(std::forward<string_t>(str));
}

auto str_to_upper(concepts::string_type auto &&str)
{
	using string_t = decltype(str);
	return detail::str_to_upper(std::forward<string_t>(str));
}

template <concepts::string_type Str>
auto str_replace(Str &&str, const str_replace_condition<get_string_char_t<Str>> &cond)
{
	using char_t = get_string_char_t<decltype(str)>;
	std::basic_string<char_t> result(std::forward<Str>(str));
	detail::str_replace<char_t>(result, cond);
	return result;
}

template <concepts::string_type Str>
auto str_replace(Str &&str, const str_replace_condition<get_string_char_t<Str>> &cond, size_t &count)
{
	using char_t = get_string_char_t<decltype(str)>;
	std::basic_string<char_t> result(std::forward<Str>(str));
	count = detail::str_replace<char_t>(result, cond);
	return result;
}

auto str_trimmed(const concepts::string_type auto &str)
{
	using char_t = get_string_char_t<decltype(str)>;
	return detail::str_trimmed<char_t>(str);
}

template <concepts::string_type Str, concepts::basic_string_type<get_string_char_t<Str>> Find>
auto str_remove(const Str &str, const Find &find, bool step)
{
	using char_t = get_string_char_t<Str>;
	return detail::str_remove<char_t>(str, find, step);
}

template <concepts::string_type Str>
auto str_remove(const Str &str, get_string_char_t<Str> find, bool step)
{
	using char_t = get_string_char_t<Str>;
	return detail::str_remove<char_t>(str, find, step);
}

auto file_name(const concepts::string_type auto &file_name)
{
	using char_t = get_string_char_t<decltype(file_name)>;
	return detail::file_name<char_t>(file_name);
}

auto file_path(const concepts::string_type auto &file_name)
{
	using char_t = get_string_char_t<decltype(file_name)>;
	return detail::file_path<char_t>(file_name);
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_BASE_H