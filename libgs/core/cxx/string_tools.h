
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

namespace libgs
{

template <concepts::weak_string_type>
struct get_string_char;

template <concepts::weak_char_string_type Str>
struct get_string_char<Str> { using type = char; };

template <concepts::weak_wchar_string_type Str>
struct get_string_char<Str> { using type = wchar_t; };

template <concepts::weak_string_type Str>
using get_string_char_t = typename get_string_char<Str>::type;

template <concepts::char_type CharT, char...Chars>
struct string_literal
{
	static constexpr std::array<CharT, sizeof...(Chars) + 1> _array {Chars..., '\0'};
	static constexpr const CharT *value = _array.data();
};

template <concepts::char_type CharT, char...Chars>
constexpr const CharT *string_literal_v = string_literal<CharT,Chars...>::value;

template <concepts::char_type CharT, char...Chars>
constexpr const CharT *s_str = string_literal_v<CharT,Chars...>;

[[nodiscard]] LIBGS_CORE_TAPI auto transition_string_view (
	concepts::weak_string_type auto &&str
);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) nosview (
	concepts::string_type auto &&str
);

[[nodiscard]] LIBGS_CORE_TAPI std::string wcstombs (
	const concepts::weak_wchar_string_type auto &str
);
[[nodiscard]] LIBGS_CORE_TAPI auto mbstowcs (
	const concepts::weak_char_string_type auto &str
);

[[nodiscard]] LIBGS_CORE_TAPI bool is_alpha(const concepts::weak_string_type auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_digit(const concepts::weak_string_type auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_rlnum(const concepts::weak_string_type auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_alnum(const concepts::weak_string_type auto &str) noexcept;
[[nodiscard]] LIBGS_CORE_TAPI bool is_ascii(const concepts::weak_string_type auto &str) noexcept;

#define LIBGS_WCHAR(s)   LIBGS_CAT(L , s)
#define LIBGS_CHAR8(s)   LIBGS_CAT(u8, s)
#define LIBGS_CHAR16(s)  LIBGS_CAT(u , s)
#define LIBGS_CHAR32(s)  LIBGS_CAT(U , s)

} //namespace libgs
#include <libgs/core/cxx/detail/string_tools.h>


#endif //LIBGS_CORE_CXX_STRING_TOOLS_H