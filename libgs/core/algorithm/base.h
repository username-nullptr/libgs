
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

#include <libgs/core/global.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_TAPI int8_t stoi8 (
	const concepts::string_type auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint8_t stou8 (
	const concepts::string_type auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int16_t stoi16 (
	const concepts::string_type auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint16_t stou16 (
	const concepts::string_type auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int32_t stoi32 (
	const concepts::string_type auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint32_t stou32 (
	const concepts::string_type auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int64_t stoi64 (
	const concepts::string_type auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint64_t stou64 (
	const concepts::string_type auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI float stof (
	const concepts::string_type auto &str
);
[[nodiscard]] LIBGS_CORE_TAPI double stod (
	const concepts::string_type auto &str
);
[[nodiscard]] LIBGS_CORE_TAPI long double stold (
	const concepts::string_type auto &str
);

[[nodiscard]] LIBGS_CORE_TAPI bool stob (
	const concepts::string_type auto &str, size_t base = 10
);

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston (
	const concepts::string_type auto &str, size_t base = 10
);

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston (
	const concepts::string_type auto &str
);

[[nodiscard]] LIBGS_CORE_TAPI int8_t stoi8_or (
	const concepts::string_type auto &str, size_t base = 10, int8_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint8_t stou8_or (
	const concepts::string_type auto &str, size_t base = 10, uint8_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int16_t stoi16_or (
	const concepts::string_type auto &str, size_t base = 10, int16_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint16_t stou16_or (
	const concepts::string_type auto &str, size_t base = 10, uint16_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int32_t stoi32_or (
	const concepts::string_type auto &str, size_t base = 10, int32_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint32_t stou32_or (
	const concepts::string_type auto &str, size_t base = 10, uint32_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int64_t stoi64_or (
	const concepts::string_type auto &str, size_t base = 10, int64_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint64_t stou64_or (
	const concepts::string_type auto &str, size_t base = 10, uint64_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI float stof_or (
	const concepts::string_type auto &str, float default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI double stod_or (
	const concepts::string_type auto &str, double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI long double stold_or (
	const concepts::string_type auto &str, long double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI bool stob_or (
	const concepts::string_type auto &str, size_t base = 10, bool default_value = false
) noexcept;

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or (
	const concepts::string_type auto &str, size_t base = 10, T default_value = 0
) noexcept;

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or (
	const concepts::string_type auto &str, T default_value = 0.0
);

[[nodiscard]] LIBGS_CORE_TAPI auto str_to_lower (
	concepts::string_type auto &&str
);
[[nodiscard]] LIBGS_CORE_TAPI auto str_to_upper (
	concepts::string_type auto &&str
);

template <concepts::char_type CharT>
struct LIBGS_CORE_TAPI str_replace_condition
{
	using char_t = CharT;
	using string_view_t = std::basic_string_view<char_t>;

	string_view_t find;
	string_view_t repl;
	bool step = true;

	str_replace_condition(string_view_t find, string_view_t repl, bool step = true) :
		find(find), repl(repl), step(step) {}

	str_replace_condition(const char &find, const char &repl, bool step = true) :
		find(&find), repl(&repl), step(step) {}

	str_replace_condition(string_view_t find, const char &repl, bool step = true) :
		find(find), repl(&repl), step(step) {}

	str_replace_condition(const char &find, string_view_t repl, bool step = true) :
		find(&find), repl(repl), step(step) {}
};

template <concepts::string_type Str>
[[nodiscard]] LIBGS_CORE_TAPI auto str_replace (
	Str &&str, const str_replace_condition<get_string_char_t<Str>> &cond
);

template <concepts::string_type Str>
[[nodiscard]] LIBGS_CORE_TAPI auto str_replace (
	Str &&str, const str_replace_condition<get_string_char_t<Str>> &cond, size_t &count
);

[[nodiscard]] LIBGS_CORE_TAPI auto str_trimmed (
	const concepts::string_type auto &str
);

template <concepts::string_type Str, concepts::basic_string_type<get_string_char_t<Str>> Find>
[[nodiscard]] LIBGS_CORE_TAPI auto str_remove (
	const Str &str, const Find &find, bool step = true
);
template <concepts::string_type Str>
[[nodiscard]] LIBGS_CORE_TAPI auto str_remove (
	const Str &str, get_string_char_t<Str> find, bool step = true
);

[[nodiscard]] LIBGS_CORE_TAPI auto file_name (
	const concepts::string_type auto &file_name
);
[[nodiscard]] LIBGS_CORE_TAPI auto file_path (
	const concepts::string_type auto &file_name
);

} //namespace libgs
#include <libgs/core/algorithm/detail/base.h>


#endif //LIBGS_CORE_ALGORITHM_BASE_H
