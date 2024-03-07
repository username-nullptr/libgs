
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

#ifndef LIBGS_CORE_VALUE_H
#define LIBGS_CORE_VALUE_H

#include <libgs/core/algorithm/base.h>
#include <format>
#include <deque>

namespace libgs
{

template <concept_char_type CharT>
class basic_value
{
public:
	template <typename...Args>
	using format_string = libgs::format_string<CharT, Args...>;

	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	static constexpr const CharT *default_format_v =
		libgs::default_format_v<CharT>;

public:
	basic_value() = default;
	basic_value(const CharT *str);

	basic_value(str_type str);
	basic_value(str_view_type str);

	template <typename Arg0, typename...Args>
	basic_value(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	template <typename T>
	basic_value(T &&v) requires (
		not requires(T &&rv) { str_type(std::forward<T>(rv)); }
	);

	basic_value(const basic_value&) = default;
	basic_value(basic_value&&) noexcept = default;

public:
	str_type &to_string();
	const str_type &to_string() const;

	operator str_type&();
	operator const str_type&() const;

public:
	template <concept_integral_type T>
	[[nodiscard]] T get(size_t base = 10) const;

	template <concept_float_type T>
	[[nodiscard]] T get() const;

	template <concept_integral_type T>
	[[nodiscard]] T get_or(size_t base = 10, T default_value = 0) const;

	template <concept_float_type T>
	[[nodiscard]] T get_or(T default_value = 0.0) const;

	template <typename T>
	[[nodiscard]] const std::basic_string<CharT> &get() const requires 
		is_dsame_v<T,std::basic_string<CharT>>;

	template <typename T>
	[[nodiscard]] std::basic_string<CharT> &get() requires 
		is_dsame_v<T,std::basic_string<CharT>>;

public:
	[[nodiscard]] bool to_bool(size_t base = 10) const;
	[[nodiscard]] int32_t to_int(size_t base = 10) const;
	[[nodiscard]] uint32_t to_uint(size_t base = 10) const;
	[[nodiscard]] int64_t to_long(size_t base = 10) const;
	[[nodiscard]] uint64_t to_ulong(size_t base = 10) const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_ldouble() const;

	[[nodiscard]] bool to_bool_or(size_t base = 10, bool default_value = false) const;
	[[nodiscard]] int32_t to_int_or(size_t base = 10, int32_t default_value = 0) const;
	[[nodiscard]] uint32_t to_uint_or(size_t base = 10, uint32_t default_value = 0) const;
	[[nodiscard]] int64_t to_long_or(size_t base = 10, int64_t default_value = 0) const;
	[[nodiscard]] uint64_t to_ulong_or(size_t base = 10, uint64_t default_value = 0) const;
	[[nodiscard]] float to_float_or(float default_value = 0.0) const;
	[[nodiscard]] double to_double_or(double default_value = 0.0) const;
	[[nodiscard]] long double to_ldouble_or(long double default_value = 0.0) const;

public:
	basic_value &set(const CharT *str);

	basic_value &set(str_type str);
	basic_value &set(str_view_type str);

	template <typename Arg0, typename...Args>
	basic_value &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	template <typename T>
	basic_value &set(T &&v) requires (
		not requires(T &&rv) { str_type(std::forward<T>(rv)); }
	);

public:
	basic_value &operator=(const basic_value&) = default;
	basic_value &operator=(basic_value&&) noexcept = default;

	bool operator<=>(const basic_value &other) const;
	bool operator<=>(const str_view_type &tr) const;
	bool operator<=>(const str_type &str) const;

public:
	basic_value &operator=(const CharT *str);
	basic_value &operator=(str_type str);
	basic_value &operator=(str_view_type str);

protected:
	std::basic_string<CharT> m_str;
};

using value = basic_value<char>;

using wvalue = basic_value<wchar_t>;

template <concept_char_type CharT>
using basic_value_list = std::deque<basic_value<CharT>>;

using value_list = basic_value_list<char>;
using wvalue_list = basic_value_list<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/value.h>


#endif //LIBGS_CORE_VALUE_H
