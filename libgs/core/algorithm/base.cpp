
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

int8_t stoi8(const std::string &str, size_t base)
{
	return algorithm_base::stoi8<char>(str, base);
}

int8_t stoi8(const std::wstring &str, size_t base)
{
	return algorithm_base::stoi8<wchar_t>(str, base);
}

uint8_t stoui8(const std::string &str, size_t base)
{
	return algorithm_base::stoui8<char>(str, base);
}

uint8_t stoui8(const std::wstring &str, size_t base)
{
	return algorithm_base::stoui8<wchar_t>(str, base);
}

int16_t stoi16(const std::string &str, size_t base)
{
	return algorithm_base::stoi16<char>(str, base);
}

int16_t stoi16(const std::wstring &str, size_t base)
{
	return algorithm_base::stoi16<wchar_t>(str, base);
}

uint16_t stoui16(const std::string &str, size_t base)
{
	return algorithm_base::stoui16<char>(str, base);
}

uint16_t stoui16(const std::wstring &str, size_t base)
{
	return algorithm_base::stoui16<wchar_t>(str, base);
}

int32_t stoi32(const std::string &str, size_t base)
{
	return algorithm_base::stoi32<char>(str, base);
}

int32_t stoi32(const std::wstring &str, size_t base)
{
	return algorithm_base::stoi32<wchar_t>(str, base);
}

uint32_t stoui32(const std::string &str, size_t base)
{
	return algorithm_base::stoui32<char>(str, base);
}

uint32_t stoui32(const std::wstring &str, size_t base)
{
	return algorithm_base::stoui32<wchar_t>(str, base);
}

int64_t stoi64(const std::string &str, size_t base)
{
	return algorithm_base::stoi64<char>(str, base);
}

int64_t stoi64(const std::wstring &str, size_t base)
{
	return algorithm_base::stoi64<wchar_t>(str, base);
}

uint64_t stoui64(const std::string &str, size_t base)
{
	return algorithm_base::stoui64<char>(str, base);
}

uint64_t stoui64(const std::wstring &str, size_t base)
{
	return algorithm_base::stoui64<wchar_t>(str, base);
}

float stof(const std::string &str)
{
	return algorithm_base::stof<char>(str);
}

float stof(const std::wstring &str)
{
	return algorithm_base::stof<wchar_t>(str);
}

double stod(const std::string &str)
{
	return algorithm_base::stod<char>(str);
}

double stod(const std::wstring &str)
{
	return algorithm_base::stod<wchar_t>(str);
}

long double stold(const std::string &str)
{
	return algorithm_base::stold<char>(str);
}

long double stold(const std::wstring &str)
{
	return algorithm_base::stold<wchar_t>(str);
}

bool stob(const std::string &str, size_t base)
{
	return algorithm_base::stob<char>(str, base);
}

bool stob(const std::wstring &str, size_t base)
{
	return algorithm_base::stob<wchar_t>(str, base);
}

int8_t stoi8_or(const std::string &str, size_t base, int8_t default_value)
{
	try {
		return libgs::stoi8(str, base);
	}
	catch(...){}
	return default_value;
}

int8_t stoi8_or(const std::wstring &str, size_t base, int8_t default_value)
{
	try {
		return libgs::stoi8(str, base);
	}
	catch(...){}
	return default_value;
}

uint8_t stoui8_or(const std::string &str, size_t base, uint8_t default_value)
{
	try {
		return libgs::stoui8(str, base);
	}
	catch(...){}
	return default_value;
}

uint8_t stoui8_or(const std::wstring &str, size_t base, uint8_t default_value)
{
	try {
		return libgs::stoui8(str, base);
	}
	catch(...){}
	return default_value;
}

int16_t stoi16_or(const std::string &str, size_t base, int16_t default_value)
{
	try {
		return libgs::stoi16(str, base);
	}
	catch(...){}
	return default_value;
}

int16_t stoi16_or(const std::wstring &str, size_t base, int16_t default_value)
{
	try {
		return libgs::stoi16(str, base);
	}
	catch(...){}
	return default_value;
}

uint16_t stoui16_or(const std::string &str, size_t base, uint16_t default_value)
{
	try {
		return libgs::stoui16(str, base);
	}
	catch(...){}
	return default_value;
}

uint16_t stoui16_or(const std::wstring &str, size_t base, uint16_t default_value)
{
	try {
		return libgs::stoui16(str, base);
	}
	catch(...){}
	return default_value;
}

int32_t stoi32_or(const std::string &str, size_t base, int32_t default_value)
{
	try {
		return libgs::stoi32(str, base);
	}
	catch(...){}
	return default_value;
}

int32_t stoi32_or(const std::wstring &str, size_t base, int32_t default_value)
{
	try {
		return libgs::stoi32(str, base);
	}
	catch(...){}
	return default_value;
}

uint32_t stoui32_or(const std::string &str, size_t base, uint32_t default_value)
{
	try {
		return libgs::stoui32(str, base);
	}
	catch(...){}
	return default_value;
}

uint32_t stoui32_or(const std::wstring &str, size_t base, uint32_t default_value)
{
	try {
		return libgs::stoui32(str, base);
	}
	catch(...){}
	return default_value;
}

int64_t stoi64_or(const std::string &str, size_t base, int64_t default_value)
{
	try {
		return libgs::stoi64(str, base);
	}
	catch(...){}
	return default_value;
}

int64_t stoi64_or(const std::wstring &str, size_t base, int64_t default_value)
{
	try {
		return libgs::stoi64(str, base);
	}
	catch(...){}
	return default_value;
}

uint64_t stoui64_or(const std::string &str, size_t base, uint64_t default_value)
{
	try {
		return libgs::stoui64(str, base);
	}
	catch(...){}
	return default_value;
}

uint64_t stoui64_or(const std::wstring &str, size_t base, uint64_t default_value)
{
	try {
		return libgs::stoui64(str, base);
	}
	catch(...){}
	return default_value;
}

float stof_or(const std::string &str, float default_value)
{
	try {
		return libgs::stof(str);
	}
	catch(...){}
	return default_value;
}

float stof_or(const std::wstring &str, float default_value)
{
	try {
		return libgs::stof(str);
	}
	catch(...){}
	return default_value;
}

double stod_or(const std::string &str, double default_value)
{
	try {
		return libgs::stod(str);
	}
	catch(...){}
	return default_value;
}

double stod_or(const std::wstring &str, double default_value)
{
	try {
		return libgs::stod(str);
	}
	catch(...){}
	return default_value;
}

long double stold_or(const std::string &str, long double default_value)
{
	try {
		return libgs::stold(str);
	}
	catch(...){}
	return default_value;
}

long double stold_or(const std::wstring &str, long double default_value)
{
	try {
		return libgs::stold(str);
	}
	catch(...){}
	return default_value;
}

bool stob_or(const std::string &str, size_t base, bool default_value)
{
	try {
		return stob(str, base);
	}
	catch(...){}
	return default_value;
}

bool stob_or(const std::wstring &str, size_t base, bool default_value)
{
	try {
		return stob(str, base);
	}
	catch(...){}
	return default_value;
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

size_t str_replace(std::string &str, std::string_view _old, std::string_view _new)
{
	return algorithm_base::str_replace<char>(str, _old, _new);
}

size_t str_replace(std::wstring &str, std::wstring_view _old, std::wstring_view _new)
{
	return algorithm_base::str_replace<wchar_t>(str, _old, _new);
}

std::string str_trimmed(std::string_view str)
{
	return algorithm_base::str_trimmed<char>(str);
}

std::wstring str_trimmed(std::wstring_view str)
{
	return algorithm_base::str_trimmed<wchar_t>(str);
}

std::string str_remove(std::string_view str, std::string_view find)
{
	return algorithm_base::str_remove<char>(str, find);
}

std::wstring str_remove(std::wstring_view str, std::wstring_view find)
{
	return algorithm_base::str_remove<wchar_t>(str, find);
}

std::string str_remove(std::string_view str, char find)
{
	return algorithm_base::str_remove<char>(str, find);
}

std::wstring str_remove(std::wstring_view str, wchar_t find)
{
	return algorithm_base::str_remove<wchar_t>(str, find);
}

std::string from_percent_encoding(std::string_view str)
{
	return algorithm_base::from_percent_encoding<char>(str);
}

std::wstring from_percent_encoding(std::wstring_view str)
{
	return algorithm_base::from_percent_encoding<wchar_t>(str);
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
