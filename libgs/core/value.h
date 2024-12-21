
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
#include <deque>

namespace libgs { namespace concepts
{

template <typename T, typename CharT>
concept vgs = std::is_same_v<T,std::basic_string<CharT>>;

}//namespace concepts

template <concepts::char_type CharT>
class LIBGS_CORE_TAPI basic_value
{
public:
	template <typename...Args>
	using format_string = libgs::format_string<CharT, Args...>;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	static constexpr const CharT *default_format_v = libgs::default_format_v<CharT>;

public:
	basic_value() = default;
	basic_value(const CharT *str);

	basic_value(string_t str);
	basic_value(string_view_t str);

	template <typename Arg0, typename...Args>
	basic_value(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	template <typename T>
	basic_value(T &&v) requires (
		not requires(T &&rv) { string_t(std::forward<T>(rv)); }
	);

	basic_value(const basic_value&) = default;
	basic_value(basic_value&&) noexcept = default;

public:
	[[nodiscard]] string_t &to_string() & noexcept;
	[[nodiscard]] const string_t &to_string() const & noexcept;
	[[nodiscard]] string_t &&to_string() && noexcept;
	[[nodiscard]] const string_t &&to_string() const && noexcept;

	operator string_t&() & noexcept;
	operator const string_t&() const & noexcept;
	operator string_t&&() && noexcept;
	operator const string_t&&() const && noexcept;

public:
	template <concepts::integral_type T>
	[[nodiscard]] T get(size_t base = 10) const;

	template <concepts::float_type T>
	[[nodiscard]] T get() const;

	template <concepts::integral_type T>
	[[nodiscard]] T get_or(size_t base = 10, T default_value = 0) const noexcept;

	template <concepts::float_type T>
	[[nodiscard]] T get_or(T default_value = 0.0) const noexcept;

	template <concepts::vgs<CharT> T = string_t>
	[[nodiscard]] const string_t &get() const & noexcept;

	template <concepts::vgs<CharT> T = string_t>
	[[nodiscard]] string_t &get() & noexcept;

	template <concepts::vgs<CharT> T = string_t>
	[[nodiscard]] const string_t &&get() const && noexcept;

	template <concepts::vgs<CharT> T = string_t>
	[[nodiscard]] string_t &&get() && noexcept;

	[[nodiscard]] const string_t &get() const & noexcept;
	[[nodiscard]] string_t &get() & noexcept;

	[[nodiscard]] const string_t &&get() const && noexcept;
	[[nodiscard]] string_t &&get() && noexcept;

	template <typename T>
	[[nodiscard]] const basic_value &get() const & noexcept
		requires std::is_same_v<T,basic_value>;

	template <typename T>
	[[nodiscard]] basic_value &get() & noexcept
		requires std::is_same_v<T,basic_value>;

	template <typename T>
	[[nodiscard]] const basic_value &&get() const && noexcept
		requires std::is_same_v<T,basic_value>;

	template <typename T>
	[[nodiscard]] basic_value &&get() && noexcept
		requires std::is_same_v<T,basic_value>;

public:
	[[nodiscard]] bool to_bool(size_t base = 10) const;
	[[nodiscard]] int32_t to_int(size_t base = 10) const;
	[[nodiscard]] uint32_t to_uint(size_t base = 10) const;
	[[nodiscard]] int64_t to_long(size_t base = 10) const;
	[[nodiscard]] uint64_t to_ulong(size_t base = 10) const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_ldouble() const;

	[[nodiscard]] bool to_bool_or(size_t base = 10, bool default_value = false) const noexcept;
	[[nodiscard]] int32_t to_int_or(size_t base = 10, int32_t default_value = 0) const noexcept;
	[[nodiscard]] uint32_t to_uint_or(size_t base = 10, uint32_t default_value = 0) const noexcept;
	[[nodiscard]] int64_t to_long_or(size_t base = 10, int64_t default_value = 0) const noexcept;
	[[nodiscard]] uint64_t to_ulong_or(size_t base = 10, uint64_t default_value = 0) const noexcept;
	[[nodiscard]] float to_float_or(float default_value = 0.0) const noexcept;
	[[nodiscard]] double to_double_or(double default_value = 0.0) const noexcept;
	[[nodiscard]] long double to_ldouble_or(long double default_value = 0.0) const noexcept;

public:
	basic_value &set(const CharT *str);

	basic_value &set(string_t str);
	basic_value &set(string_view_t str);

	template <typename Arg0, typename...Args>
	basic_value &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	template <typename T>
	basic_value &set(T &&v) requires (
		not requires(T &&rv) { string_t(std::forward<T>(rv)); }
	);

public:
	[[nodiscard]] bool is_alpha() const noexcept;
	[[nodiscard]] bool is_digit() const noexcept;
	[[nodiscard]] bool is_alnum() const noexcept;

public:
	[[nodiscard]] string_t &operator*() & noexcept;
	[[nodiscard]] const string_t &operator*() const & noexcept;
	[[nodiscard]] string_t &&operator*() && noexcept;
	[[nodiscard]] const string_t &&operator*() const && noexcept;

	string_t *operator->() noexcept;
	const string_t *operator->() const noexcept;

public:
	[[nodiscard]] bool operator==(const basic_value &other) const = default;
	[[nodiscard]] bool operator==(const string_view_t &tr) const;
	[[nodiscard]] bool operator==(const string_t &str) const;

	[[nodiscard]] auto operator<=>(const basic_value &other) const;
	[[nodiscard]] auto operator<=>(const string_view_t &tr) const;
	[[nodiscard]] auto operator<=>(const string_t &str) const;

public:
	basic_value &operator=(const basic_value&) = default;
	basic_value &operator=(basic_value&&) noexcept = default;

	basic_value &operator=(const CharT *str);
	basic_value &operator=(string_t str);
	basic_value &operator=(string_view_t str);

protected:
	string_t m_str;
};

using value = basic_value<char>;
using wvalue = basic_value<wchar_t>;

template <concepts::char_type CharT>
using basic_value_list = std::deque<basic_value<CharT>>;

using value_list = basic_value_list<char>;
using wvalue_list = basic_value_list<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/value.h>


#endif //LIBGS_CORE_VALUE_H
