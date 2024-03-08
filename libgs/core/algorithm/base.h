
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

[[nodiscard]] long double stold(const std::string &str);
[[nodiscard]] long double stold(const std::wstring &str);

[[nodiscard]] bool stob(const std::string &str, size_t base = 10);
[[nodiscard]] bool stob(const std::wstring &str, size_t base = 10);

template <concept_integral_type T>
[[nodiscard]] T ston(const std::string &str, size_t base = 10) {
	return algorithm_base::ston<T,char>(str, base);
}

template <concept_integral_type T>
[[nodiscard]] T ston(const std::wstring &str, size_t base = 10) {
	return algorithm_base::ston<T,wchar_t>(str, base);
}

template <concept_float_type T>
[[nodiscard]] T ston(const std::string &str) {
	return algorithm_base::ston<T,char>(str);
}

template <concept_float_type T>
[[nodiscard]] T ston(const std::wstring &str) {
	return algorithm_base::ston<T,wchar_t>(str);
}

[[nodiscard]] int8_t stoi8_or(const std::string &str, size_t base = 10, int8_t default_value = 0) noexcept;
[[nodiscard]] int8_t stoi8_or(const std::wstring &str, size_t base = 10, int8_t default_value = 0) noexcept;

[[nodiscard]] uint8_t stoui8_or(const std::string &str, size_t base = 10, uint8_t default_value = 0) noexcept;
[[nodiscard]] uint8_t stoui8_or(const std::wstring &str, size_t base = 10, uint8_t default_value = 0) noexcept;

[[nodiscard]] int16_t stoi16_or(const std::string &str, size_t base = 10, int16_t default_value = 0) noexcept;
[[nodiscard]] int16_t stoi16_or(const std::wstring &str, size_t base = 10, int16_t default_value = 0) noexcept;

[[nodiscard]] uint16_t stoui16_or(const std::string &str, size_t base = 10, uint16_t default_value = 0) noexcept;
[[nodiscard]] uint16_t stoui16_or(const std::wstring &str, size_t base = 10, uint16_t default_value = 0) noexcept;

[[nodiscard]] int32_t stoi32_or(const std::string &str, size_t base = 10, int32_t default_value = 0) noexcept;
[[nodiscard]] int32_t stoi32_or(const std::wstring &str, size_t base = 10, int32_t default_value = 0) noexcept;

[[nodiscard]] uint32_t stoui32_or(const std::string &str, size_t base = 10, uint32_t default_value = 0) noexcept;
[[nodiscard]] uint32_t stoui32_or(const std::wstring &str, size_t base = 10, uint32_t default_value = 0) noexcept;

[[nodiscard]] int64_t stoi64_or(const std::string &str, size_t base = 10, int64_t default_value = 0) noexcept;
[[nodiscard]] int64_t stoi64_or(const std::wstring &str, size_t base = 10, int64_t default_value = 0) noexcept;

[[nodiscard]] uint64_t stoui64_or(const std::string &str, size_t base = 10, uint64_t default_value = 0) noexcept;
[[nodiscard]] uint64_t stoui64_or(const std::wstring &str, size_t base = 10, uint64_t default_value = 0) noexcept;

[[nodiscard]] float stof_or(const std::string &str, float default_value = 0.0) noexcept;
[[nodiscard]] float stof_or(const std::wstring &str, float default_value = 0.0) noexcept;

[[nodiscard]] double stod_or(const std::string &str, double default_value = 0.0) noexcept;
[[nodiscard]] double stod_or(const std::wstring &str, double default_value = 0.0) noexcept;

[[nodiscard]] long double stold_or(const std::string &str, long double default_value = 0.0) noexcept;
[[nodiscard]] long double stold_or(const std::wstring &str, long double default_value = 0.0) noexcept;

[[nodiscard]] bool stob_or(const std::string &str, size_t base = 10, bool default_value = false) noexcept;
[[nodiscard]] bool stob_or(const std::wstring &str, size_t base = 10, bool default_value = false) noexcept;

template <concept_integral_type T>
[[nodiscard]] T ston_or(const std::string &str, size_t base = 10, T default_value = 0) noexcept {
	return algorithm_base::ston<T,char>(str, base, default_value);
}

template <concept_integral_type T>
[[nodiscard]] T ston_or(const std::wstring &str, size_t base = 10, T default_value = 0) noexcept {
	return algorithm_base::ston<T,wchar_t>(str, base, default_value);
}

template <concept_float_type T>
[[nodiscard]] T ston_or(const std::string &str, T default_value = 0.0) {
	return algorithm_base::ston<T,char>(str, default_value);
}

template <concept_float_type T>
[[nodiscard]] T ston_or(const std::wstring &str, T default_value = 0.0) {
	return algorithm_base::ston<T,wchar_t>(str, default_value);
}

[[nodiscard]] std::string str_to_lower(std::string_view str);
[[nodiscard]] std::wstring str_to_lower(std::wstring_view str);

[[nodiscard]] std::string str_to_upper(std::string_view str);
[[nodiscard]] std::wstring str_to_upper(std::wstring_view str);

/*[[nodiscard]]*/ size_t str_replace(std::string &str, std::string_view _old, std::string_view _new);
/*[[nodiscard]]*/ size_t str_replace(std::wstring &str, std::wstring_view _old, std::wstring_view _new);

[[nodiscard]] std::string str_trimmed(std::string_view str);
[[nodiscard]] std::wstring str_trimmed(std::wstring_view str);

[[nodiscard]] std::string str_remove(std::string_view str, std::string_view find);
[[nodiscard]] std::wstring str_remove(std::wstring_view str, std::wstring_view find);

[[nodiscard]] std::string str_remove(std::string_view str, char find);
[[nodiscard]] std::wstring str_remove(std::wstring_view str, wchar_t find);

[[nodiscard]] std::string from_percent_encoding(std::string_view str);
[[nodiscard]] std::wstring from_percent_encoding(std::wstring_view str);

[[nodiscard]] std::string file_name(std::string_view file_name);
[[nodiscard]] std::wstring file_name(std::wstring_view file_name);

[[nodiscard]] std::string file_path(std::string_view file_name);
[[nodiscard]] std::wstring file_path(std::wstring_view file_name);

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_BASE_H
