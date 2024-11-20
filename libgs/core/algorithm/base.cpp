
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

#include "base.h"

namespace libgs { namespace detail
{

template <concepts::char_type CharT>
[[nodiscard]] static int _stob(std::basic_string_view<CharT> str)
{
	if constexpr( is_char_v<CharT> )
	{
#ifdef WIN32
		if( _stricmp(str.data(), "true") == 0 )
			return 1;
		else if( _stricmp(str.data(), "false") == 0 )
			return 0;
#else
		if( str.size() == 4 and strncasecmp(str.data(), "true", 4) == 0 )
			return 1;
		else if( str.size() == 5 and strncasecmp(str.data(), "false", 5) == 0 )
			return 0;
#endif
	}
	else
		return _stob<char>(wcstombs(str));
	return -1;
}

template <concepts::char_type CharT, typename T>
[[nodiscard]] static T try_stobtot(std::basic_string_view<CharT> str, const std::optional<T> &odv)
{
	int res = _stob<CharT>(str);
	if( res < 0 )
	{
		if( odv )
			return *odv;
		throw runtime_error("Cannot convert string to arithmetic.");
	}
	return static_cast<T>(!!res);
}

template <concepts::char_type CharT>
[[nodiscard]] static auto _sto_float(auto &&func, std::basic_string_view<CharT> str)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index);
	if( index < str.size() )
		throw runtime_error("Cannot convert string to arithmetic.");
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] static auto _sto_int(auto &&func, std::basic_string_view<CharT> str, size_t base)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index, static_cast<int>(base));
	if( index < str.size() )
	{
		res = static_cast<decltype(res)>(
			_sto_float(static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str)
		);
	}
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] static int8_t stoi8(std::basic_string_view<CharT> str, size_t base, std::optional<int8_t> odv = {})
{
	try {
		return static_cast<int8_t>(
			_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base)
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static uint8_t stoui8(std::basic_string_view<CharT> str, size_t base, std::optional<uint8_t> odv = {})
{
	try {
		return static_cast<uint8_t>(_sto_int(
			static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static int16_t stoi16(std::basic_string_view<CharT> str, size_t base, std::optional<int16_t> odv = {})
{
	try {
		return static_cast<int16_t>(_sto_int(
			static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static uint16_t stoui16(std::basic_string_view<CharT> str, size_t base, std::optional<uint16_t> odv = {})
{
	try {
		return static_cast<uint16_t>(_sto_int(
			static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static int32_t stoi32(std::basic_string_view<CharT> str, size_t base, std::optional<int32_t> odv = {})
{
	try {
		return static_cast<int32_t>(_sto_int(
			static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static uint32_t stoui32(std::basic_string_view<CharT> str, size_t base, std::optional<uint32_t> odv = {})
{
	try {
		return static_cast<uint32_t>(_sto_int(
			static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static int64_t stoi64(std::basic_string_view<CharT> str, size_t base, std::optional<int64_t> odv = {})
{
	try {
		return static_cast<int64_t>(_sto_int(
			static_cast<long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoll), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static uint64_t stoui64(std::basic_string_view<CharT> str, size_t base, std::optional<uint64_t> odv = {})
{
	try {
		return static_cast<uint64_t>(_sto_int(
			static_cast<unsigned long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoull), str, base
		));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static float stof(std::basic_string_view<CharT> str, std::optional<float> odv = {})
{
	try {
		return _sto_float(
			static_cast<float(*)(const std::basic_string<CharT>&,size_t*)>(std::stof), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static double stod(std::basic_string_view<CharT> str, std::optional<double> odv = {})
{
	try {
		return _sto_float(
			static_cast<double(*)(const std::basic_string<CharT>&,size_t*)>(std::stod), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static double stold(std::basic_string_view<CharT> str, std::optional<double> odv = {})
{
	try {
		return _sto_float(
			static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str
		);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concepts::char_type CharT>
[[nodiscard]] static bool stob(std::basic_string_view<CharT> str, size_t base, std::optional<bool> odv = {})
{
	int res = _stob<CharT>(str);
	if( res < 0 )
	{
		try {
			return /*!!*/_sto_int(
				static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
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

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> str_to_lower(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result(str.size(), '\0');
	for(size_t i=0; i<str.size(); i++)
		result[i] = ::tolower(str[i]);
	return result;
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> str_to_upper(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result(str.size(), '\0');
	for(size_t i=0; i<str.size(); i++)
		result[i] = ::toupper(str[i]);
	return result;
}

template <concepts::char_type CharT>
static size_t str_replace(
	std::basic_string<CharT> &str, std::basic_string_view<CharT> _old, std::basic_string_view<CharT> _new, bool step
){
	if( _old == _new )
		return 0;

	size_t sum = 0;
	size_t old_pos = 0;

	for(;;)
	{
		auto start = str.find(_old, old_pos);
		if( start == std::basic_string<CharT>::npos )
			break;

		str.replace(start, _old.size(), _new);
		old_pos = start;

		if( step )
			old_pos += _new.size();
		sum++;
	}
	return sum;
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> str_trimmed(std::basic_string_view<CharT> str)
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
	while( right >= 0 )
	{
		if( str[right] >= 1 and str[right] <= 32 )
			right--;
		else
			break;
	}
	if( right < 0 )
		return result;

	result = str.substr(0, right + 1);
	result = result.substr(left);
	return result;
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT>
str_remove(std::basic_string_view<CharT> str, std::basic_string_view<CharT> find, bool step)
{
	std::basic_string<CharT> res(str.data(), str.size());
	str_replace<CharT>(res, find, std::basic_string<CharT>(), step);
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> str_remove(std::basic_string_view<CharT> str, CharT find, bool)
{
	std::basic_string<CharT> res(str.data(), str.size());
	auto it = std::remove(res.begin(), res.end(), find);
	if( it != res.end() )
		res.erase(it, res.end());
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] static std::basic_string<CharT> file_name(std::basic_string_view<CharT> file_name)
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
[[nodiscard]] static std::basic_string<CharT> file_path(std::basic_string_view<CharT> file_name)
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

int8_t stoi8(std::string_view str, size_t base)
{
	return detail::stoi8<char>(str, base);
}

int8_t stoi8(std::wstring_view str, size_t base)
{
	return detail::stoi8<wchar_t>(str, base);
}

uint8_t stoui8(std::string_view str, size_t base)
{
	return detail::stoui8<char>(str, base);
}

uint8_t stoui8(std::wstring_view str, size_t base)
{
	return detail::stoui8<wchar_t>(str, base);
}

int16_t stoi16(std::string_view str, size_t base)
{
	return detail::stoi16<char>(str, base);
}

int16_t stoi16(std::wstring_view str, size_t base)
{
	return detail::stoi16<wchar_t>(str, base);
}

uint16_t stoui16(std::string_view str, size_t base)
{
	return detail::stoui16<char>(str, base);
}

uint16_t stoui16(std::wstring_view str, size_t base)
{
	return detail::stoui16<wchar_t>(str, base);
}

int32_t stoi32(std::string_view str, size_t base)
{
	return detail::stoi32<char>(str, base);
}

int32_t stoi32(std::wstring_view str, size_t base)
{
	return detail::stoi32<wchar_t>(str, base);
}

uint32_t stoui32(std::string_view str, size_t base)
{
	return detail::stoui32<char>(str, base);
}

uint32_t stoui32(std::wstring_view str, size_t base)
{
	return detail::stoui32<wchar_t>(str, base);
}

int64_t stoi64(std::string_view str, size_t base)
{
	return detail::stoi64<char>(str, base);
}

int64_t stoi64(std::wstring_view str, size_t base)
{
	return detail::stoi64<wchar_t>(str, base);
}

uint64_t stoui64(std::string_view str, size_t base)
{
	return detail::stoui64<char>(str, base);
}

uint64_t stoui64(std::wstring_view str, size_t base)
{
	return detail::stoui64<wchar_t>(str, base);
}

float stof(std::string_view str)
{
	return detail::stof<char>(str);
}

float stof(std::wstring_view str)
{
	return detail::stof<wchar_t>(str);
}

double stod(std::string_view str)
{
	return detail::stod<char>(str);
}

double stod(std::wstring_view str)
{
	return detail::stod<wchar_t>(str);
}

long double stold(std::string_view str)
{
	return detail::stold<char>(str);
}

long double stold(std::wstring_view str)
{
	return detail::stold<wchar_t>(str);
}

bool stob(std::string_view str, size_t base)
{
	return detail::stob<char>(str, base);
}

bool stob(std::wstring_view str, size_t base)
{
	return detail::stob<wchar_t>(str, base);
}

int8_t stoi8_or(std::string_view str, size_t base, int8_t default_value) noexcept
{
	return detail::stoi8<char>(str, base, default_value);
}

int8_t stoi8_or(std::wstring_view str, size_t base, int8_t default_value) noexcept
{
	return detail::stoi8<wchar_t>(str, base, default_value);
}

uint8_t stoui8_or(std::string_view str, size_t base, uint8_t default_value) noexcept
{
	return detail::stoui8<char>(str, base, default_value);
}

uint8_t stoui8_or(std::wstring_view str, size_t base, uint8_t default_value) noexcept
{
	return detail::stoui8<wchar_t>(str, base, default_value);
}

int16_t stoi16_or(std::string_view str, size_t base, int16_t default_value) noexcept
{
	return detail::stoi16<char>(str, base, default_value);
}

int16_t stoi16_or(std::wstring_view str, size_t base, int16_t default_value) noexcept
{
	return detail::stoi16<wchar_t>(str, base, default_value);
}

uint16_t stoui16_or(std::string_view str, size_t base, uint16_t default_value) noexcept
{
	return detail::stoui16<char>(str, base, default_value);
}

uint16_t stoui16_or(std::wstring_view str, size_t base, uint16_t default_value) noexcept
{
	return detail::stoui16<wchar_t>(str, base, default_value);
}

int32_t stoi32_or(std::string_view str, size_t base, int32_t default_value) noexcept
{
	return detail::stoi32<char>(str, base, default_value);
}

int32_t stoi32_or(std::wstring_view str, size_t base, int32_t default_value) noexcept
{
	return detail::stoi32<wchar_t>(str, base, default_value);
}

uint32_t stoui32_or(std::string_view str, size_t base, uint32_t default_value) noexcept
{
	return detail::stoui32<char>(str, base, default_value);
}

uint32_t stoui32_or(std::wstring_view str, size_t base, uint32_t default_value) noexcept
{
	return detail::stoui32<wchar_t>(str, base, default_value);
}

int64_t stoi64_or(std::string_view str, size_t base, int64_t default_value) noexcept
{
	return detail::stoi64<char>(str, base, default_value);
}

int64_t stoi64_or(std::wstring_view str, size_t base, int64_t default_value) noexcept
{
	return detail::stoi64<wchar_t>(str, base, default_value);
}

uint64_t stoui64_or(std::string_view str, size_t base, uint64_t default_value) noexcept
{
	return detail::stoui64<char>(str, base, default_value);
}

uint64_t stoui64_or(std::wstring_view str, size_t base, uint64_t default_value) noexcept
{
	return detail::stoui64<wchar_t>(str, base, default_value);
}

float stof_or(std::string_view str, float default_value) noexcept
{
	return detail::stof<char>(str, default_value);
}

float stof_or(std::wstring_view str, float default_value) noexcept
{
	return detail::stof<wchar_t>(str, default_value);
}

double stod_or(std::string_view str, double default_value) noexcept
{
	return detail::stod<char>(str, default_value);
}

double stod_or(std::wstring_view str, double default_value) noexcept
{
	return detail::stod<wchar_t>(str, default_value);
}

long double stold_or(std::string_view str, long double default_value) noexcept
{
	return detail::stold<char>(str, default_value);
}

long double stold_or(std::wstring_view str, long double default_value) noexcept
{
	return detail::stold<wchar_t>(str, default_value);
}

bool stob_or(std::string_view str, size_t base, bool default_value) noexcept
{
	return detail::stob<char>(str, base, default_value);
}

bool stob_or(std::wstring_view str, size_t base, bool default_value) noexcept
{
	return detail::stob<wchar_t>(str, base, default_value);
}

std::string str_to_lower(std::string_view str)
{
	return detail::str_to_lower<char>(str);
}

std::wstring str_to_lower(std::wstring_view str)
{
	return detail::str_to_lower<wchar_t>(str);
}

std::string str_to_upper(std::string_view str)
{
	return detail::str_to_upper<char>(str);
}

std::wstring str_to_upper(std::wstring_view str)
{
	return detail::str_to_upper<wchar_t>(str);
}

size_t str_replace(std::string &str, std::string_view _old, std::string_view _new, bool step)
{
	return detail::str_replace<char>(str, _old, _new, step);
}

size_t str_replace(std::wstring &str, std::wstring_view _old, std::wstring_view _new, bool step)
{
	return detail::str_replace<wchar_t>(str, _old, _new, step);
}

size_t str_replace(std::string &str, std::string_view _old, char _new, bool step)
{
	return str_replace(str, _old, {&_new,1}, step);
}

size_t str_replace(std::wstring &str, std::wstring_view _old, wchar_t _new, bool step)
{
	return str_replace(str, _old, {&_new,1}, step);
}

size_t str_replace(std::string &str, char _old, std::string_view _new, bool step)
{
	return str_replace(str, {&_old,1}, _new, step);
}

size_t str_replace(std::wstring &str, wchar_t _old, std::wstring_view _new, bool step)
{
	return str_replace(str, {&_old,1}, _new, step);
}

size_t str_replace(std::string &str, char _old, char _new, bool step)
{
	return str_replace(str, {&_old,1}, {&_new,1}, step);
}

size_t str_replace(std::wstring &str, wchar_t _old, wchar_t _new, bool step)
{
	return str_replace(str, {&_old,1}, {&_new,1}, step);
}

std::string str_trimmed(std::string_view str)
{
	return detail::str_trimmed<char>(str);
}

std::wstring str_trimmed(std::wstring_view str)
{
	return detail::str_trimmed<wchar_t>(str);
}

std::string str_remove(std::string_view str, std::string_view find, bool step)
{
	return detail::str_remove<char>(str, find, step);
}

std::wstring str_remove(std::wstring_view str, std::wstring_view find, bool step)
{
	return detail::str_remove<wchar_t>(str, find, step);
}

std::string str_remove(std::string_view str, char find, bool step)
{
	return detail::str_remove<char>(str, find, step);
}

std::wstring str_remove(std::wstring_view str, wchar_t find, bool step)
{
	return detail::str_remove<wchar_t>(str, find, step);
}

std::string file_name(std::string_view file_name)
{
	return detail::file_name<char>(file_name);
}

std::wstring file_name(std::wstring_view file_name)
{
	return detail::file_name<wchar_t>(file_name);
}

std::string file_path(std::string_view file_name)
{
	return detail::file_path<char>(file_name);
}

std::wstring file_path(std::wstring_view file_name)
{
	return detail::file_path<wchar_t>(file_name);
}

} //namespace libgs
