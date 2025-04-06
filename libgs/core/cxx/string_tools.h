
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

#ifndef LIBGS_CORE_CXX_STRING_TOOLS_H
#define LIBGS_CORE_CXX_STRING_TOOLS_H

#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/concepts.h>

namespace libgs::strtls
{

#define LIBGS_WCHAR(s)   LIBGS_CAT(L , s)
#define LIBGS_CHAR8(s)   LIBGS_CAT(u8, s)
#define LIBGS_CHAR16(s)  LIBGS_CAT(u , s)
#define LIBGS_CHAR32(s)  LIBGS_CAT(U , s)

#define string_literal(_type, _str) \
	[] <concepts::character CharT> () consteval { \
			 if constexpr( is_wchar_v <CharT> ) return  L##_str; \
		else if constexpr( is_char8_v <CharT> ) return u8##_str; \
		else if constexpr( is_char16_v<CharT> ) return  u##_str; \
		else if constexpr( is_char32_v<CharT> ) return  U##_str; \
		else									return     _str; \
	} .template operator()<_type>()

#define l_str(_type, _str)  string_literal(_type, _str)

template <concepts::any_text_p>
struct get_char;

template <concepts::text_p<char> Text>
struct get_char<Text> { using type = char; };

template <concepts::text_p<wchar_t> Text>
struct get_char<Text> { using type = wchar_t; };

template <concepts::text_p<char8_t> Text>
struct get_char<Text> { using type = char8_t; };

template <concepts::text_p<char16_t> Text>
struct get_char<Text> { using type = char16_t; };

template <concepts::text_p<char32_t> Text>
struct get_char<Text> { using type = char32_t; };

template <concepts::any_text_p Text>
using get_char_t = typename get_char<Text>::type;

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) to_view (
	concepts::any_text auto &&str
);

[[nodiscard]] LIBGS_CORE_TAPI bool is_alpha(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_digit(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_rlnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_alnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_ascii(const concepts::any_string_p auto &str) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int8_t to_int8 (
	const concepts::any_text_p auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint8_t to_uint8 (
	const concepts::any_text_p auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int16_t to_int16 (
	const concepts::any_text_p auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint16_t to_uint16 (
	const concepts::any_text_p auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int32_t to_int32 (
	const concepts::any_text_p auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint32_t to_uint32 (
	const concepts::any_text_p auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int64_t to_int64 (
	const concepts::any_text_p auto &str, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint64_t to_uint64 (
	const concepts::any_text_p auto &str, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI float to_float (
	const concepts::any_text_p auto &str
);
[[nodiscard]] LIBGS_CORE_TAPI double to_double (
	const concepts::any_text_p auto &str
);
[[nodiscard]] LIBGS_CORE_TAPI long double to_ldouble (
	const concepts::any_text_p auto &str
);

[[nodiscard]] LIBGS_CORE_TAPI bool to_bool (
	const concepts::any_text_p auto &str, size_t base = 10
);

template <concepts::integral_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith (
	const concepts::any_text_p auto &str, size_t base = 10
);

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith (
	const concepts::any_text_p auto &str
);

[[nodiscard]] LIBGS_CORE_TAPI int8_t to_int8_or (
	const concepts::any_text_p auto &str, size_t base = 10, int8_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint8_t to_uint8_or (
	const concepts::any_text_p auto &str, size_t base = 10, uint8_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int16_t to_int16_or (
	const concepts::any_text_p auto &str, size_t base = 10, int16_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint16_t to_uint16_or (
	const concepts::any_text_p auto &str, size_t base = 10, uint16_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int32_t to_int32_or (
	const concepts::any_text_p auto &str, size_t base = 10, int32_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint32_t to_uint32_or (
	const concepts::any_text_p auto &str, size_t base = 10, uint32_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int64_t to_int64_or (
	const concepts::any_text_p auto &str, size_t base = 10, int64_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint64_t to_uint64_or (
	const concepts::any_text_p auto &str, size_t base = 10, uint64_t default_value = 0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI float to_float_or (
	const concepts::any_text_p auto &str, float default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI double to_double_or (
	const concepts::any_text_p auto &str, double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI long double to_ldouble_or (
	const concepts::any_text_p auto &str, long double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI bool to_bool_or (
	const concepts::any_text_p auto &str, size_t base = 10, bool default_value = false
) noexcept;

template <concepts::integral_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith_or (
	const concepts::any_text_p auto &str, size_t base = 10, T default_value = 0
) noexcept;

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith_or (
	const concepts::any_text_p auto &str, T default_value = 0.0
);

[[nodiscard]] LIBGS_CORE_TAPI auto to_lower (
	concepts::any_text_p auto &&str
);
[[nodiscard]] LIBGS_CORE_TAPI auto to_upper (
	concepts::any_text_p auto &&str
);

template <concepts::any_string_p Str>
struct LIBGS_CORE_TAPI str_replace_condition
{
	using char_t = get_char_t<Str>;
	using string_view_t = std::basic_string_view<char_t>;

	string_view_t optd;
	string_view_t find;
	string_view_t repl;
	bool step = true;

	str_replace_condition (
		Str &&optd, Str &&find, Str &&repl, bool step = true
	);
};

template <concepts::any_string_p Str>
[[nodiscard]] LIBGS_CORE_TAPI auto replace (
	const str_replace_condition<Str> &cond
);

template <concepts::any_string_p Str>
[[nodiscard]] LIBGS_CORE_TAPI auto replace (
	const str_replace_condition<Str> &cond, size_t &count
);

[[nodiscard]] LIBGS_CORE_TAPI auto trimmed (
	const concepts::any_text_p auto &str
);

template <concepts::any_string_p Str, concepts::text_p<get_char_t<Str>> Find>
[[nodiscard]] LIBGS_CORE_TAPI auto remove (
	const Str &str, const Find &find, bool step = true
);

} //namespace libgs::strtls
#include <libgs/core/cxx/detail/string_tools.h>


#endif //LIBGS_CORE_CXX_STRING_TOOLS_H