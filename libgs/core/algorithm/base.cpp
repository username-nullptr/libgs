
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

namespace libgs
{

int8_t stoi8(std::string_view str, size_t base)
{
	return algorithm_base::stoi8<char>(str, base);
}

int8_t stoi8(std::wstring_view str, size_t base)
{
	return algorithm_base::stoi8<wchar_t>(str, base);
}

uint8_t stoui8(std::string_view str, size_t base)
{
	return algorithm_base::stoui8<char>(str, base);
}

uint8_t stoui8(std::wstring_view str, size_t base)
{
	return algorithm_base::stoui8<wchar_t>(str, base);
}

int16_t stoi16(std::string_view str, size_t base)
{
	return algorithm_base::stoi16<char>(str, base);
}

int16_t stoi16(std::wstring_view str, size_t base)
{
	return algorithm_base::stoi16<wchar_t>(str, base);
}

uint16_t stoui16(std::string_view str, size_t base)
{
	return algorithm_base::stoui16<char>(str, base);
}

uint16_t stoui16(std::wstring_view str, size_t base)
{
	return algorithm_base::stoui16<wchar_t>(str, base);
}

int32_t stoi32(std::string_view str, size_t base)
{
	return algorithm_base::stoi32<char>(str, base);
}

int32_t stoi32(std::wstring_view str, size_t base)
{
	return algorithm_base::stoi32<wchar_t>(str, base);
}

uint32_t stoui32(std::string_view str, size_t base)
{
	return algorithm_base::stoui32<char>(str, base);
}

uint32_t stoui32(std::wstring_view str, size_t base)
{
	return algorithm_base::stoui32<wchar_t>(str, base);
}

int64_t stoi64(std::string_view str, size_t base)
{
	return algorithm_base::stoi64<char>(str, base);
}

int64_t stoi64(std::wstring_view str, size_t base)
{
	return algorithm_base::stoi64<wchar_t>(str, base);
}

uint64_t stoui64(std::string_view str, size_t base)
{
	return algorithm_base::stoui64<char>(str, base);
}

uint64_t stoui64(std::wstring_view str, size_t base)
{
	return algorithm_base::stoui64<wchar_t>(str, base);
}

float stof(std::string_view str)
{
	return algorithm_base::stof<char>(str);
}

float stof(std::wstring_view str)
{
	return algorithm_base::stof<wchar_t>(str);
}

double stod(std::string_view str)
{
	return algorithm_base::stod<char>(str);
}

double stod(std::wstring_view str)
{
	return algorithm_base::stod<wchar_t>(str);
}

long double stold(std::string_view str)
{
	return algorithm_base::stold<char>(str);
}

long double stold(std::wstring_view str)
{
	return algorithm_base::stold<wchar_t>(str);
}

bool stob(std::string_view str, size_t base)
{
	return algorithm_base::stob<char>(str, base);
}

bool stob(std::wstring_view str, size_t base)
{
	return algorithm_base::stob<wchar_t>(str, base);
}

int8_t stoi8_or(std::string_view str, size_t base, int8_t default_value) noexcept
{
	return algorithm_base::stoi8<char>(str, base, default_value);
}

int8_t stoi8_or(std::wstring_view str, size_t base, int8_t default_value) noexcept
{
	return algorithm_base::stoi8<wchar_t>(str, base, default_value);
}

uint8_t stoui8_or(std::string_view str, size_t base, uint8_t default_value) noexcept
{
	return algorithm_base::stoui8<char>(str, base, default_value);
}

uint8_t stoui8_or(std::wstring_view str, size_t base, uint8_t default_value) noexcept
{
	return algorithm_base::stoui8<wchar_t>(str, base, default_value);
}

int16_t stoi16_or(std::string_view str, size_t base, int16_t default_value) noexcept
{
	return algorithm_base::stoi16<char>(str, base, default_value);
}

int16_t stoi16_or(std::wstring_view str, size_t base, int16_t default_value) noexcept
{
	return algorithm_base::stoi16<wchar_t>(str, base, default_value);
}

uint16_t stoui16_or(std::string_view str, size_t base, uint16_t default_value) noexcept
{
	return algorithm_base::stoui16<char>(str, base, default_value);
}

uint16_t stoui16_or(std::wstring_view str, size_t base, uint16_t default_value) noexcept
{
	return algorithm_base::stoui16<wchar_t>(str, base, default_value);
}

int32_t stoi32_or(std::string_view str, size_t base, int32_t default_value) noexcept
{
	return algorithm_base::stoi32<char>(str, base, default_value);
}

int32_t stoi32_or(std::wstring_view str, size_t base, int32_t default_value) noexcept
{
	return algorithm_base::stoi32<wchar_t>(str, base, default_value);
}

uint32_t stoui32_or(std::string_view str, size_t base, uint32_t default_value) noexcept
{
	return algorithm_base::stoui32<char>(str, base, default_value);
}

uint32_t stoui32_or(std::wstring_view str, size_t base, uint32_t default_value) noexcept
{
	return algorithm_base::stoui32<wchar_t>(str, base, default_value);
}

int64_t stoi64_or(std::string_view str, size_t base, int64_t default_value) noexcept
{
	return algorithm_base::stoi64<char>(str, base, default_value);
}

int64_t stoi64_or(std::wstring_view str, size_t base, int64_t default_value) noexcept
{
	return algorithm_base::stoi64<wchar_t>(str, base, default_value);
}

uint64_t stoui64_or(std::string_view str, size_t base, uint64_t default_value) noexcept
{
	return algorithm_base::stoui64<char>(str, base, default_value);
}

uint64_t stoui64_or(std::wstring_view str, size_t base, uint64_t default_value) noexcept
{
	return algorithm_base::stoui64<wchar_t>(str, base, default_value);
}

float stof_or(std::string_view str, float default_value) noexcept
{
	return algorithm_base::stof<char>(str, default_value);
}

float stof_or(std::wstring_view str, float default_value) noexcept
{
	return algorithm_base::stof<wchar_t>(str, default_value);
}

double stod_or(std::string_view str, double default_value) noexcept
{
	return algorithm_base::stod<char>(str, default_value);
}

double stod_or(std::wstring_view str, double default_value) noexcept
{
	return algorithm_base::stod<wchar_t>(str, default_value);
}

long double stold_or(std::string_view str, long double default_value) noexcept
{
	return algorithm_base::stold<char>(str, default_value);
}

long double stold_or(std::wstring_view str, long double default_value) noexcept
{
	return algorithm_base::stold<wchar_t>(str, default_value);
}

bool stob_or(std::string_view str, size_t base, bool default_value) noexcept
{
	return algorithm_base::stob<char>(str, base, default_value);
}

bool stob_or(std::wstring_view str, size_t base, bool default_value) noexcept
{
	return algorithm_base::stob<wchar_t>(str, base, default_value);
}

std::string str_to_lower(std::string_view str)
{
	return algorithm_base::str_to_lower<char>(str);
}

std::wstring str_to_lower(std::wstring_view str)
{
	return algorithm_base::str_to_lower<wchar_t>(str);
}

std::string str_to_upper(std::string_view str)
{
	return algorithm_base::str_to_upper<char>(str);
}

std::wstring str_to_upper(std::wstring_view str)
{
	return algorithm_base::str_to_upper<wchar_t>(str);
}

size_t str_replace(std::string &str, std::string_view _old, std::string_view _new, bool step)
{
	return algorithm_base::str_replace<char>(str, _old, _new, step);
}

size_t str_replace(std::wstring &str, std::wstring_view _old, std::wstring_view _new, bool step)
{
	return algorithm_base::str_replace<wchar_t>(str, _old, _new, step);
}

size_t str_replace(std::string &str, std::string_view _old, char _new, bool step)
{
	return str_replace(str, std::move(_old), {&_new,1}, step);
}

size_t str_replace(std::wstring &str, std::wstring_view _old, wchar_t _new, bool step)
{
	return str_replace(str, std::move(_old), {&_new,1}, step);
}

size_t str_replace(std::string &str, char _old, std::string_view _new, bool step)
{
	return str_replace(str, {&_old,1}, std::move(_new), step);
}

size_t str_replace(std::wstring &str, wchar_t _old, std::wstring_view _new, bool step)
{
	return str_replace(str, {&_old,1}, std::move(_new), step);
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
	return algorithm_base::str_trimmed<char>(str);
}

std::wstring str_trimmed(std::wstring_view str)
{
	return algorithm_base::str_trimmed<wchar_t>(str);
}

std::string str_remove(std::string_view str, std::string_view find, bool step)
{
	return algorithm_base::str_remove<char>(str, find, step);
}

std::wstring str_remove(std::wstring_view str, std::wstring_view find, bool step)
{
	return algorithm_base::str_remove<wchar_t>(str, find, step);
}

std::string str_remove(std::string_view str, char find, bool step)
{
	return algorithm_base::str_remove<char>(str, find, step);
}

std::wstring str_remove(std::wstring_view str, wchar_t find, bool step)
{
	return algorithm_base::str_remove<wchar_t>(str, find, step);
}

std::string from_percent_encoding(std::string_view str)
{
	return algorithm_base::from_percent_encoding<char>(str);
}

std::wstring from_percent_encoding(std::wstring_view str)
{
	return algorithm_base::from_percent_encoding<wchar_t>(str);
}

std::string to_percent_encoding(std::string_view str, std::string_view exclude, std::string_view include, char percent)
{
	return algorithm_base::to_percent_encoding<char>(str, exclude, include, percent);
}

std::wstring to_percent_encoding(std::wstring_view str, std::wstring_view exclude, std::wstring_view include, wchar_t percent)
{
	return algorithm_base::to_percent_encoding<wchar_t>(str, exclude, include, percent);
}

size_t wildcard_match(std::string_view rule, std::string_view str)
{
	return algorithm_base::wildcard_match<char>(rule, str);
}

size_t wildcard_match(std::wstring_view rule, std::wstring_view str)
{
	return algorithm_base::wildcard_match<wchar_t>(rule, str);
}

std::string file_name(std::string_view file_name)
{
	return algorithm_base::file_name<char>(file_name);
}

std::wstring file_name(std::wstring_view file_name)
{
	return algorithm_base::file_name<wchar_t>(file_name);
}

std::string file_path(std::string_view file_name)
{
	return algorithm_base::file_path<char>(file_name);
}

std::wstring file_path(std::wstring_view file_name)
{
	return algorithm_base::file_path<wchar_t>(file_name);
}

} //namespace libgs
