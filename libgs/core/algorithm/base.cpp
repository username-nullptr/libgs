
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

double stold(const std::string &str)
{
	return algorithm_base::stold<char>(str);
}

double stold(const std::wstring &str)
{
	return algorithm_base::stold<wchar_t>(str);
}

bool stob(const std::string &str)
{
	return algorithm_base::stob<char>(str);
}

bool stob(const std::wstring &str)
{
	return algorithm_base::stob<wchar_t>(str);
}

std::string str_to_lower(const std::string &str)
{
	return algorithm_base::str_to_lower<char>(str);
}

std::wstring str_to_lower(const std::wstring &str)
{
	return algorithm_base::str_to_lower<wchar_t>(str);
}

std::string str_to_upper(const std::string &str)
{
	return algorithm_base::str_to_upper<char>(str);
}

std::wstring str_to_upper(const std::wstring &str)
{
	return algorithm_base::str_to_upper<wchar_t>(str);
}

size_t str_replace(std::string &str, const std::string &_old, const std::string &_new)
{
	return algorithm_base::str_replace<char>(str, _old, _new);
}

size_t str_replace(std::wstring &str, const std::wstring &_old, const std::wstring &_new)
{
	return algorithm_base::str_replace<wchar_t>(str, _old, _new);
}

std::string str_trimmed(const std::string &str)
{
	return algorithm_base::str_trimmed<char>(str);
}

std::wstring str_trimmed(const std::wstring &str)
{
	return algorithm_base::str_trimmed<wchar_t>(str);
}

std::string str_remove(const std::string &str, const std::string &find)
{
	return algorithm_base::str_remove<char>(str, find);
}

std::wstring str_remove(const std::wstring &str, const std::wstring &find)
{
	return algorithm_base::str_remove<wchar_t>(str, find);
}

std::string str_remove(const std::string &str, char find)
{
	return algorithm_base::str_remove<char>(str, find);
}

std::wstring str_remove(const std::wstring &str, wchar_t find)
{
	return algorithm_base::str_remove<wchar_t>(str, find);
}

std::string from_percent_encoding(const std::string &str)
{
	return algorithm_base::from_percent_encoding<char>(str);
}

std::wstring from_percent_encoding(const std::wstring &str)
{
	return algorithm_base::from_percent_encoding<wchar_t>(str);
}

std::string file_name(const std::string &file_name)
{
	return algorithm_base::file_name<char>(file_name);
}

std::wstring file_name(const std::wstring &file_name)
{
	return algorithm_base::file_name<wchar_t>(file_name);
}

std::string file_path(const std::string &file_name)
{
	return algorithm_base::file_path<char>(file_name);
}

std::wstring file_path(const std::wstring &file_name)
{
	return algorithm_base::file_path<wchar_t>(file_name);
}

} //namespace libgs
