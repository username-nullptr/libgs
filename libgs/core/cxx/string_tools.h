
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

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) nosview(concepts::string_type auto &&str);

[[nodiscard]] LIBGS_CORE_VAPI std::string wcstombs(std::wstring_view str);
[[nodiscard]] LIBGS_CORE_VAPI char wcstombs(wchar_t c);

[[nodiscard]] LIBGS_CORE_VAPI std::wstring mbstowcs(std::string_view str);
[[nodiscard]] LIBGS_CORE_VAPI wchar_t mbstowcs(char c);

LIBGS_CORE_TAPI decltype(auto) xxtombs(concepts::string_type auto &&str);
LIBGS_CORE_TAPI char xxtombs(concepts::char_type auto c);

LIBGS_CORE_TAPI decltype(auto) xxtowcs(concepts::string_type auto &&str);
LIBGS_CORE_TAPI wchar_t xxtowcs(concepts::char_type auto c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI decltype(auto) mbstoxx(concepts::char_string_type auto &&str);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI CharT mbstoxx(char c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI decltype(auto) wcstoxx(concepts::wchar_string_type auto &&str);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI CharT wcstoxx(wchar_t c);

struct LIBGS_CORE_VAPI string_wrapper
{
	std::string value;

	string_wrapper() = default;
	string_wrapper(const char *value);
	string_wrapper(const wchar_t *value);

	string_wrapper(const std::string &value);
	string_wrapper(std::string &&value);

	string_wrapper(std::string_view value);
	string_wrapper(std::wstring_view value);

	operator std::string&();
	operator const std::string&() const;

	std::string &operator*();
	std::string *operator->();
};

LIBGS_CORE_VAPI [[nodiscard]] bool is_alpha(std::string_view str) noexcept;
LIBGS_CORE_VAPI [[nodiscard]] bool is_digit(std::string_view str) noexcept;
LIBGS_CORE_VAPI [[nodiscard]] bool is_alnum(std::string_view str) noexcept;
LIBGS_CORE_VAPI [[nodiscard]] bool is_ascii(std::string_view str) noexcept;

} //namespace libgs
#include <libgs/core/cxx/detail/string_tools.h>


#endif //LIBGS_CORE_CXX_STRING_TOOLS_H