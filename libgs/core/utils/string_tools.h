
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_UTILS_STRING_TOOLS_H
#define LIBGS_CORE_UTILS_STRING_TOOLS_H

#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/optional.h>

namespace libgs::strtls
{

#define LIBGS_WCHAR(s)   LIBGS_CAT(L , s)
#define LIBGS_CHAR8(s)   LIBGS_CAT(u8, s)
#define LIBGS_CHAR16(s)  LIBGS_CAT(u , s)
#define LIBGS_CHAR32(s)  LIBGS_CAT(U , s)

#ifdef string_literal
# undef string_literal
#endif
#define string_literal(_type, _str) \
	[] <libgs::concepts::character __CharT_> () consteval { \
			 if constexpr( libgs::is_wchar_v <__CharT_> ) return  L##_str; \
		else if constexpr( libgs::is_char8_v <__CharT_> ) return u8##_str; \
		else if constexpr( libgs::is_char16_v<__CharT_> ) return  u##_str; \
		else if constexpr( libgs::is_char32_v<__CharT_> ) return  U##_str; \
		else                                              return     _str; \
	} .template operator()<_type>()

#ifdef l_str
# undef l_str
#endif
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

template <concepts::character CharT = char>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> to_string (
	concepts::integral_p auto &&value, size_t base = 10, bool uppercase = false
);

template <concepts::character CharT = char>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT> to_string (
	concepts::floating_p auto &&value
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) to_string (
	concepts::any_text_p auto &&text
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) to_view (
	concepts::any_text_p auto &&text
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI bool is_alpha(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_digit(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_rlnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_alnum(const concepts::any_string_p auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_ascii(const concepts::any_string_p auto &str) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<int8_t> to_int8 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<uint8_t> to_uint8 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<int16_t> to_int16 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<uint16_t> to_uint16 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<int32_t> to_int32 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<uint32_t> to_uint32 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<int64_t> to_int64 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<uint64_t> to_uint64 (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<float> to_float (
	const concepts::any_text_p auto &text
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<double> to_double (
	const concepts::any_text_p auto &text
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<long double> to_ldouble (
	const concepts::any_text_p auto &text
) noexcept;

[[nodiscard]] LIBGS_CORE_TAPI optional<bool> to_bool (
	const concepts::any_text_p auto &text, size_t base = 10
) noexcept;

template <typename T>
[[nodiscard]] LIBGS_CORE_TAPI optional<T> to_arith(const concepts::any_text_p auto &text, size_t base = 10)
	noexcept requires concepts::integral_p<T> or concepts::enumerate_p<T>;

template <concepts::floating_p T>
[[nodiscard]] LIBGS_CORE_TAPI optional<T> to_arith (
	const concepts::any_text_p auto &text
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