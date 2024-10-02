
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

#include <libgs/core/algorithm/detail/basic_base.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_API int8_t stoi8(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API int8_t stoi8(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API uint8_t stoui8(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API uint8_t stoui8(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API int16_t stoi16(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API int16_t stoi16(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API uint16_t stoui16(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API uint16_t stoui16(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API int32_t stoi32(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API int32_t stoi32(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API uint32_t stoui32(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API uint32_t stoui32(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API int64_t stoi64(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API int64_t stoi64(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API uint64_t stoui64(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API uint64_t stoui64(std::wstring_view str, size_t base = 10);

[[nodiscard]] LIBGS_CORE_API float stof(std::string_view str);
[[nodiscard]] LIBGS_CORE_API float stof(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API double stod(std::string_view str);
[[nodiscard]] LIBGS_CORE_API double stod(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API long double stold(std::string_view str);
[[nodiscard]] LIBGS_CORE_API long double stold(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API bool stob(std::string_view str, size_t base = 10);
[[nodiscard]] LIBGS_CORE_API bool stob(std::wstring_view str, size_t base = 10);

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::string_view str, size_t base = 10) {
	return algorithm_base::ston<T,char>(str, base);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::wstring_view str, size_t base = 10) {
	return algorithm_base::ston<T,wchar_t>(str, base);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::string_view str) {
	return algorithm_base::ston<T,char>(str);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::wstring_view str) {
	return algorithm_base::ston<T,wchar_t>(str);
}

[[nodiscard]] LIBGS_CORE_API int8_t stoi8_or(std::string_view str, size_t base = 10, int8_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API int8_t stoi8_or(std::wstring_view str, size_t base = 10, int8_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API uint8_t stoui8_or(std::string_view str, size_t base = 10, uint8_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API uint8_t stoui8_or(std::wstring_view str, size_t base = 10, uint8_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API int16_t stoi16_or(std::string_view str, size_t base = 10, int16_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API int16_t stoi16_or(std::wstring_view str, size_t base = 10, int16_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API uint16_t stoui16_or(std::string_view str, size_t base = 10, uint16_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API uint16_t stoui16_or(std::wstring_view str, size_t base = 10, uint16_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API int32_t stoi32_or(std::string_view str, size_t base = 10, int32_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API int32_t stoi32_or(std::wstring_view str, size_t base = 10, int32_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API uint32_t stoui32_or(std::string_view str, size_t base = 10, uint32_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API uint32_t stoui32_or(std::wstring_view str, size_t base = 10, uint32_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API int64_t stoi64_or(std::string_view str, size_t base = 10, int64_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API int64_t stoi64_or(std::wstring_view str, size_t base = 10, int64_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API uint64_t stoui64_or(std::string_view str, size_t base = 10, uint64_t default_value = 0) noexcept;
[[nodiscard]] LIBGS_CORE_API uint64_t stoui64_or(std::wstring_view str, size_t base = 10, uint64_t default_value = 0) noexcept;

[[nodiscard]] LIBGS_CORE_API float stof_or(std::string_view str, float default_value = 0.0) noexcept;
[[nodiscard]] LIBGS_CORE_API float stof_or(std::wstring_view str, float default_value = 0.0) noexcept;

[[nodiscard]] LIBGS_CORE_API double stod_or(std::string_view str, double default_value = 0.0) noexcept;
[[nodiscard]] LIBGS_CORE_API double stod_or(std::wstring_view str, double default_value = 0.0) noexcept;

[[nodiscard]] LIBGS_CORE_API long double stold_or(std::string_view str, long double default_value = 0.0) noexcept;
[[nodiscard]] LIBGS_CORE_API long double stold_or(std::wstring_view str, long double default_value = 0.0) noexcept;

[[nodiscard]] LIBGS_CORE_API bool stob_or(std::string_view str, size_t base = 10, bool default_value = false) noexcept;
[[nodiscard]] LIBGS_CORE_API bool stob_or(std::wstring_view str, size_t base = 10, bool default_value = false) noexcept;

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::string_view str, size_t base = 10, T default_value = 0) noexcept {
	return algorithm_base::ston<T,char>(str, base, default_value);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::wstring_view str, size_t base = 10, T default_value = 0) noexcept {
	return algorithm_base::ston<T,wchar_t>(str, base, default_value);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::string_view str, T default_value = 0.0) {
	return algorithm_base::ston<T,char>(str, default_value);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::wstring_view str, T default_value = 0.0) {
	return algorithm_base::ston<T,wchar_t>(str, default_value);
}

[[nodiscard]] LIBGS_CORE_API std::string str_to_lower(std::string_view str);
[[nodiscard]] LIBGS_CORE_API std::wstring str_to_lower(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API std::string str_to_upper(std::string_view str);
[[nodiscard]] LIBGS_CORE_API std::wstring str_to_upper(std::wstring_view str);

/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::string &str, std::string_view _old, std::string_view _new, bool step = true);
/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::wstring &str, std::wstring_view _old, std::wstring_view _new, bool step = true);

/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::string &str, std::string_view _old, char _new, bool step = true);
/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::wstring &str, std::wstring_view _old, wchar_t _new, bool step = true);

/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::string &str, char _old, std::string_view _new, bool step = true);
/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::wstring &str, wchar_t _old, std::wstring_view _new, bool step = true);

/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::string &str, char _old, char _new, bool step = true);
/*[[nodiscard]]*/ LIBGS_CORE_API size_t str_replace(std::wstring &str, wchar_t _old, wchar_t _new, bool step = true);

[[nodiscard]] LIBGS_CORE_API std::string str_trimmed(std::string_view str);
[[nodiscard]] LIBGS_CORE_API std::wstring str_trimmed(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API std::string str_remove(std::string_view str, std::string_view find, bool step = true);
[[nodiscard]] LIBGS_CORE_API std::wstring str_remove(std::wstring_view str, std::wstring_view find, bool step = true);

[[nodiscard]] LIBGS_CORE_API std::string str_remove(std::string_view str, char find, bool step = true);
[[nodiscard]] LIBGS_CORE_API std::wstring str_remove(std::wstring_view str, wchar_t find, bool step = true);

[[nodiscard]] LIBGS_CORE_API std::string from_percent_encoding(std::string_view str);
[[nodiscard]] LIBGS_CORE_API std::wstring from_percent_encoding(std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API std::string to_percent_encoding
(std::string_view str, std::string_view exclude = {}, std::string_view include = {}, char percent = '%');

[[nodiscard]] LIBGS_CORE_API std::wstring to_percent_encoding
(std::wstring_view str, std::wstring_view exclude = {}, std::wstring_view include = {}, char percent = '%');

[[nodiscard]] LIBGS_CORE_API int32_t wildcard_match(std::string_view rule, std::string_view str);
[[nodiscard]] LIBGS_CORE_API int32_t wildcard_match(std::wstring_view rule, std::wstring_view str);

[[nodiscard]] LIBGS_CORE_API std::string file_name(std::string_view file_name);
[[nodiscard]] LIBGS_CORE_API std::wstring file_name(std::wstring_view file_name);

[[nodiscard]] LIBGS_CORE_API std::string file_path(std::string_view file_name);
[[nodiscard]] LIBGS_CORE_API std::wstring file_path(std::wstring_view file_name);

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_BASE_H
