
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

#ifndef LIBGS_CORE_ALGORITHM_BASE_H
#define LIBGS_CORE_ALGORITHM_BASE_H

#include <libgs/core/algorithm/detail/base.h>

namespace libgs
{

[[nodiscard]] int8_t stoi8(const std::string &str, size_t base = 10);
[[nodiscard]] int8_t stoi8(const std::wstring &str, size_t base = 10);

[[nodiscard]] uint8_t stoui8(const std::string &str, size_t base = 10);
[[nodiscard]] uint8_t stoui8(const std::wstring &str, size_t base = 10);

[[nodiscard]] int16_t stoi16(const std::string &str, size_t base = 10);
[[nodiscard]] int16_t stoi16(const std::wstring &str, size_t base = 10);

[[nodiscard]] uint16_t stoui16(const std::string &str, size_t base = 10);
[[nodiscard]] uint16_t stoui16(const std::wstring &str, size_t base = 10);

[[nodiscard]] int32_t stoi32(const std::string &str, size_t base = 10);
[[nodiscard]] int32_t stoi32(const std::wstring &str, size_t base = 10);

[[nodiscard]] uint32_t stoui32(const std::string &str, size_t base = 10);
[[nodiscard]] uint32_t stoui32(const std::wstring &str, size_t base = 10);

[[nodiscard]] int64_t stoi64(const std::string &str, size_t base = 10);
[[nodiscard]] int64_t stoi64(const std::wstring &str, size_t base = 10);

[[nodiscard]] uint64_t stoui64(const std::string &str, size_t base = 10);
[[nodiscard]] uint64_t stoui64(const std::wstring &str, size_t base = 10);

[[nodiscard]] float stof(const std::string &str);
[[nodiscard]] float stof(const std::wstring &str);

[[nodiscard]] double stod(const std::string &str);
[[nodiscard]] double stod(const std::wstring &str);

[[nodiscard]] double stold(const std::string &str);
[[nodiscard]] double stold(const std::wstring &str);

[[nodiscard]] bool stob(const std::string &str);
[[nodiscard]] bool stob(const std::wstring &str);

template <concept_number_type T>
[[nodiscard]] T ston(const std::string &str, size_t base = 10) {
	return algorithm_base::ston<T,char>(str, base);
}

template <concept_number_type T>
[[nodiscard]] T ston(const std::wstring &str, size_t base = 10) {
	return algorithm_base::ston<T,wchar_t>(str, base);
}

[[nodiscard]] std::string str_to_lower(const std::string &str);
[[nodiscard]] std::wstring str_to_lower(const std::wstring &str);

[[nodiscard]] std::string str_to_upper(const std::string &str);
[[nodiscard]] std::wstring str_to_upper(const std::wstring &str);

/*[[nodiscard]]*/ size_t str_replace(std::string &str, const std::string &_old, const std::string &_new);
/*[[nodiscard]]*/ size_t str_replace(std::wstring &str, const std::wstring &_old, const std::wstring &_new);

[[nodiscard]] std::string str_trimmed(const std::string &str);
[[nodiscard]] std::wstring str_trimmed(const std::wstring &str);

[[nodiscard]] std::string str_remove(const std::string &str, const std::string &find);
[[nodiscard]] std::wstring str_remove(const std::wstring &str, const std::wstring &find);

[[nodiscard]] std::string str_remove(const std::string &str, char find);
[[nodiscard]] std::wstring str_remove(const std::wstring &str, wchar_t find);

[[nodiscard]] std::string from_percent_encoding(const std::string &str);
[[nodiscard]] std::wstring from_percent_encoding(const std::wstring &str);

[[nodiscard]] std::string file_name(const std::string &file_name);
[[nodiscard]] std::wstring file_name(const std::wstring &file_name);

[[nodiscard]] std::string file_path(const std::string &file_name);
[[nodiscard]] std::wstring file_path(const std::wstring &file_name);

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_BASE_H
