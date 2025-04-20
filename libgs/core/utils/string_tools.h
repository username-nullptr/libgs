
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

#ifndef LIBGS_CORE_UTILS_STRING_TOOLS_H
#define LIBGS_CORE_UTILS_STRING_TOOLS_H

#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>

namespace libgs::strtls
{

#define LIBGS_WCHAR(s)   LIBGS_CAT(L , s)
#define LIBGS_CHAR8(s)   LIBGS_CAT(u8, s)
#define LIBGS_CHAR16(s)  LIBGS_CAT(u , s)
#define LIBGS_CHAR32(s)  LIBGS_CAT(U , s)

#define string_literal(_type, _str) \
	[] <libgs::concepts::character __CharT_> () consteval { \
			 if constexpr( libgs::is_wchar_v <__CharT_> ) return  L##_str; \
		else if constexpr( libgs::is_char8_v <__CharT_> ) return u8##_str; \
		else if constexpr( libgs::is_char16_v<__CharT_> ) return  u##_str; \
		else if constexpr( libgs::is_char32_v<__CharT_> ) return  U##_str; \
		else                                              return     _str; \
	} .template operator()<_type>()

#define l_str(_type, _str)  string_literal(_type, _str)

template <typename>
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

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) to_string (
	concepts::any_text_p auto &&text
);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) to_view (
	concepts::any_text_p auto &&text
);

[[nodiscard]] LIBGS_CORE_TAPI bool is_alpha(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_digit(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_rlnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_alnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_ascii(const concepts::any_string_p auto &str) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int8_t to_int8 (
	const concepts::any_text_p auto &text, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint8_t to_uint8 (
	const concepts::any_text_p auto &text, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int16_t to_int16 (
	const concepts::any_text_p auto &text, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint16_t to_uint16 (
	const concepts::any_text_p auto &text, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int32_t to_int32 (
	const concepts::any_text_p auto &text, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint32_t to_uint32 (
	const concepts::any_text_p auto &text, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI int64_t to_int64 (
	const concepts::any_text_p auto &text, size_t base = 10
);
[[nodiscard]] LIBGS_CORE_TAPI uint64_t to_uint64 (
	const concepts::any_text_p auto &text, size_t base = 10
);

[[nodiscard]] LIBGS_CORE_TAPI float to_float (
	const concepts::any_text_p auto &text
);
[[nodiscard]] LIBGS_CORE_TAPI double to_double (
	const concepts::any_text_p auto &text
);
[[nodiscard]] LIBGS_CORE_TAPI long double to_ldouble (
	const concepts::any_text_p auto &text
);

[[nodiscard]] LIBGS_CORE_TAPI bool to_bool (
	const concepts::any_text_p auto &text, size_t base = 10
);

template <concepts::integral_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith (
	const concepts::any_text_p auto &text, size_t base = 10
);

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith (
	const concepts::any_text_p auto &text
);

[[nodiscard]] LIBGS_CORE_TAPI int8_t to_int8_or (
	const concepts::any_text_p auto &text, int8_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint8_t to_uint8_or (
	const concepts::any_text_p auto &text, uint8_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int16_t to_int16_or (
	const concepts::any_text_p auto &text, int16_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint16_t to_uint16_or (
	const concepts::any_text_p auto &text, uint16_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int32_t to_int32_or (
	const concepts::any_text_p auto &text, int32_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint32_t to_uint32_or (
	const concepts::any_text_p auto &text, uint32_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI int64_t to_int64_or (
	const concepts::any_text_p auto &text, int64_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI uint64_t to_uint64_or (
	const concepts::any_text_p auto &text, uint64_t default_value = 0, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI float to_float_or (
	const concepts::any_text_p auto &text, float default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI double to_double_or (
	const concepts::any_text_p auto &text, double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI long double to_ldouble_or (
	const concepts::any_text_p auto &text, long double default_value = 0.0
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI bool to_bool_or (
	const concepts::any_text_p auto &text, bool default_value = false, size_t base = 10
) noexcept;

template <concepts::integral_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith_or (
	const concepts::any_text_p auto &text, T default_value = static_cast<T>(0), size_t base = 10
) noexcept;

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI T to_arith_or (
	const concepts::any_text_p auto &text, T default_value = static_cast<T>(0)
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI auto to_lower (
	concepts::any_text_p auto &&text
);
[[nodiscard]] LIBGS_CORE_TAPI auto to_upper (
	concepts::any_text_p auto &&text
);

template <concepts::any_string_p Str>
[[nodiscard]] LIBGS_CORE_TAPI auto replace (
	Str &&str,
	concepts::text_p<get_char_t<Str>> auto &&find, concepts::text_p<get_char_t<Str>> auto &&repl,
	bool step = true
);

template <concepts::any_string_p Str>
[[nodiscard]] LIBGS_CORE_TAPI auto replace (
	size_t &count, Str &&str,
	concepts::text_p<get_char_t<Str>> auto &&find, concepts::text_p<get_char_t<Str>> auto &&repl,
	bool step = true
);

[[nodiscard]] LIBGS_CORE_TAPI auto trimmed (
	const concepts::any_text_p auto &text
);

template <concepts::any_string_p Str, concepts::text_p<get_char_t<Str>> Find>
[[nodiscard]] LIBGS_CORE_TAPI auto remove (
	const Str &str, const Find &find, bool step = true
);

[[nodiscard]] LIBGS_CORE_TAPI auto file_name (
	const concepts::any_text_p auto &file_name
);

[[nodiscard]] LIBGS_CORE_TAPI auto file_path (
	const concepts::any_text_p auto &file_name
);

} //namespace libgs::strtls
#include <libgs/core/utils/detail/string_tools.h>


#endif //LIBGS_CORE_UTILS_STRING_TOOLS_H