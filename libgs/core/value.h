
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

namespace libgs
{

template <concepts::char_type CharT>
class basic_value;

namespace concepts
{

template <typename T, typename CharT>
concept basic_text_arg = []() consteval -> bool
{
	using tt = std::remove_cvref_t<T>;
	return is_basic_string_v<T,CharT> or
		   std::is_base_of_v<basic_value<CharT>,tt> or
		   std::is_arithmetic_v<tt> or
		   std::is_enum_v<tt>;
}();

template <typename T>
concept text_arg = basic_text_arg<T,char>;

template <typename T>
concept wtext_arg = basic_text_arg<T,wchar_t>;

template <typename T, typename CharT>
concept basic_value_arg = basic_text_arg<T,CharT> or requires(T &&rv) {
	std::format(default_format_v<CharT>, std::forward<T>(rv));
};

template <typename T>
concept value_arg = basic_value_arg<T,char>;

template <typename T>
concept wvalue_arg = basic_value_arg<T,wchar_t>;

template <typename T, typename CharT>
concept basic_rvgs =
	std::is_same_v<T,std::basic_string<CharT>> or
	std::is_same_v<T,basic_value<CharT>>;

template <typename T>
concept rvgs = basic_rvgs<T,char>;

template <typename T>
concept wrvgs = basic_rvgs<T,wchar_t>;

template <typename T, typename CharT>
concept basic_vgs = basic_rvgs<T,CharT> or std::is_same_v<T,basic_value<CharT>>;

template <typename T>
concept vgs = basic_vgs<T,char>;

template <typename T>
concept wvgs = basic_vgs<T,wchar_t>;

}//namespace concepts

template <concepts::char_type CharT>
class LIBGS_CORE_TAPI basic_value
{
public:
	using char_t = CharT;
	using string_t = std::basic_string<CharT>;
	using str_view_t = std::basic_string_view<CharT>;

	template <typename...Args>
	using format_string = libgs::format_string<CharT, Args...>;

public:
	basic_value() = default;
	basic_value(concepts::basic_value_arg<char_t> auto &&arg);

	template <typename Arg0, typename...Args>
	basic_value(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

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
	[[nodiscard]] T get_or(size_t base = 10, T def_value = 0) const noexcept;

	template <concepts::float_type T>
	[[nodiscard]] T get_or(T def_value = 0.0) const noexcept;

	template <concepts::basic_vgs<CharT> T = string_t>
	[[nodiscard]] decltype(auto) get() & noexcept;

	template <concepts::basic_rvgs<CharT> T = string_t>
	[[nodiscard]] decltype(auto) get() && noexcept;

	template <concepts::basic_vgs<CharT> T = string_t>
	[[nodiscard]] auto &&get() const & noexcept;

	template <concepts::basic_rvgs<CharT> T = string_t>
	[[nodiscard]] auto &&get() const && noexcept;

	[[nodiscard]] string_t &get() & noexcept;
	[[nodiscard]] string_t &&get() && noexcept;

	[[nodiscard]] const string_t &get() const & noexcept;
	[[nodiscard]] const string_t &&get() const && noexcept;

public:
	[[nodiscard]] bool to_bool(size_t base = 10) const;
	[[nodiscard]] int32_t to_int(size_t base = 10) const;
	[[nodiscard]] uint32_t to_uint(size_t base = 10) const;
	[[nodiscard]] int64_t to_long(size_t base = 10) const;
	[[nodiscard]] uint64_t to_ulong(size_t base = 10) const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_ldouble() const;

	[[nodiscard]] bool to_bool_or(size_t base = 10, bool def_value = false) const noexcept;
	[[nodiscard]] int32_t to_int_or(size_t base = 10, int32_t def_value = 0) const noexcept;
	[[nodiscard]] uint32_t to_uint_or(size_t base = 10, uint32_t def_value = 0) const noexcept;
	[[nodiscard]] int64_t to_long_or(size_t base = 10, int64_t def_value = 0) const noexcept;
	[[nodiscard]] uint64_t to_ulong_or(size_t base = 10, uint64_t def_value = 0) const noexcept;
	[[nodiscard]] float to_float_or(float def_value = 0.0) const noexcept;
	[[nodiscard]] double to_double_or(double def_value = 0.0) const noexcept;
	[[nodiscard]] long double to_ldouble_or(long double def_value = 0.0) const noexcept;

public:
	template <typename Arg0, typename...Args>
	void set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	void set(concepts::basic_value_arg<char_t> auto &&arg);

public:
	[[nodiscard]] bool is_alpha() const noexcept;
	[[nodiscard]] bool is_digit() const noexcept;
	[[nodiscard]] bool is_rlnum() const noexcept;
	[[nodiscard]] bool is_alnum() const noexcept;
	[[nodiscard]] bool is_ascii() const noexcept;

public:
	[[nodiscard]] string_t &operator*() & noexcept;
	[[nodiscard]] const string_t &operator*() const & noexcept;
	[[nodiscard]] string_t &&operator*() && noexcept;
	[[nodiscard]] const string_t &&operator*() const && noexcept;

	string_t *operator->() noexcept;
	const string_t *operator->() const noexcept;

public:
	[[nodiscard]] bool operator==(const basic_value &other) const = default;
	[[nodiscard]] bool operator==(const str_view_t &tr) const;
	[[nodiscard]] bool operator==(const string_t &str) const;

	[[nodiscard]] auto operator<=>(const basic_value &other) const;
	[[nodiscard]] auto operator<=>(const str_view_t &tr) const;
	[[nodiscard]] auto operator<=>(const string_t &str) const;

public:
	basic_value &operator=(concepts::basic_value_arg<char_t> auto &&arg);
	basic_value &operator=(const basic_value&) = default;
	basic_value &operator=(basic_value&&) noexcept = default;

protected:
	string_t m_str;
};

using value = basic_value<char>;
using wvalue = basic_value<wchar_t>;

template <concepts::char_type CharT>
using basic_value_list = std::deque<basic_value<CharT>>;

using value_list = basic_value_list<char>;
using wvalue_list = basic_value_list<wchar_t>;

template <concepts::char_type CharT>
using basic_value_optl = std::optional<basic_value<CharT>>;

using value_optl = basic_value_optl<char>;
using wvalue_optl = basic_value_optl<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/value.h>


#endif //LIBGS_CORE_VALUE_H
