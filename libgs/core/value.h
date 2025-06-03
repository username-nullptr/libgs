
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

#include <libgs/core/detail/value_format.h>

namespace libgs
{

template <concepts::character CharT, typename Traits, typename Alloc>
class LIBGS_CORE_TAPI basic_value
{
public:
	using char_t = CharT;
	static_assert (
		is_char_v<char_t> or is_wchar_v<char_t> or
		is_char8_v<char_t> or is_char16_v<char_t> or is_char32_v<char_t>,
		"CharT must be a character type (char, wchar_t, char8_t, char16_t, char32_t)."
	);
	using traits_t = Traits;
	using allocator_t = Alloc;

	using string_t = std::basic_string<char_t,traits_t,allocator_t>;
	using str_view_t = std::basic_string_view<char_t,traits_t>;

	template <typename...Args>
	using format_string = std::basic_format_string <
		char_t, std::type_identity_t<Args>...
	>;

public:
	basic_value() = default;
	basic_value(concepts::value_set<char_t> auto &&arg);

	template <typename Arg0, typename...Args>
	basic_value(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	basic_value(const basic_value&) = default;
	basic_value(basic_value&&) noexcept = default;

public:
	[[nodiscard]] string_t &to_string() & noexcept;
	[[nodiscard]] const string_t &to_string() const & noexcept;
	[[nodiscard]] string_t to_string() && noexcept;

	operator string_t&() & noexcept;
	operator const string_t&() const & noexcept;
	operator string_t() && noexcept;

public:
	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get(Args&&...args) &
		requires concepts::value_get<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get(Args&&...args) &&
		requires concepts::value_get<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get(Args&&...args) const &
		requires concepts::value_get<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get(Args&&...args) const &&
		requires concepts::value_get<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get_or(T &&def_value = T(), Args&&...args) &
		requires concepts::value_get_or<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get_or(T &&def_value = T(), Args&&...args) &&
		requires concepts::value_get_or<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get_or(T &&def_value = T(), Args&&...args) const &
		requires concepts::value_get_or<T,CharT,Args...>;

	template <typename T, typename...Args>
	[[nodiscard]] decltype(auto) get_or(T &&def_value = T(), Args&&...args) const &&
		requires concepts::value_get_or<T,CharT,Args...>;

	[[nodiscard]] string_t &get() & noexcept;
	[[nodiscard]] const string_t &get() const & noexcept;
	[[nodiscard]] string_t &&get() && noexcept;

public:
	[[nodiscard]] bool to_bool(size_t base = 10) const;
	[[nodiscard]] int32_t to_int(size_t base = 10) const;
	[[nodiscard]] uint32_t to_uint(size_t base = 10) const;
	[[nodiscard]] int64_t to_long(size_t base = 10) const;
	[[nodiscard]] uint64_t to_ulong(size_t base = 10) const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_ldouble() const;

	[[nodiscard]] bool to_bool_or(bool def_value = false, size_t base = 10) const noexcept;
	[[nodiscard]] int32_t to_int_or(int32_t def_value = 0, size_t base = 10) const noexcept;
	[[nodiscard]] uint32_t to_uint_or(uint32_t def_value = 0, size_t base = 10) const noexcept;
	[[nodiscard]] int64_t to_long_or(int64_t def_value = 0, size_t base = 10) const noexcept;
	[[nodiscard]] uint64_t to_ulong_or(uint64_t def_value = 0, size_t base = 10) const noexcept;
	[[nodiscard]] float to_float_or(float def_value = 0.0) const noexcept;
	[[nodiscard]] double to_double_or(double def_value = 0.0) const noexcept;
	[[nodiscard]] long double to_ldouble_or(long double def_value = 0.0) const noexcept;

public:
	template <typename Arg0, typename...Args>
	basic_value &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	basic_value &set(concepts::value_set<char_t> auto &&arg);

public:
	[[nodiscard]] bool is_alpha() const noexcept;
	[[nodiscard]] bool is_digit() const noexcept;
	[[nodiscard]] bool is_rlnum() const noexcept;
	[[nodiscard]] bool is_alnum() const noexcept;
	[[nodiscard]] bool is_ascii() const noexcept;

public:
	[[nodiscard]] string_t &operator*() & noexcept;
	[[nodiscard]] const string_t &operator*() const & noexcept;
	[[nodiscard]] string_t operator*() && noexcept;

	string_t *operator->() noexcept;
	const string_t *operator->() const noexcept;

public:
	[[nodiscard]] bool operator==(const basic_value &other) const = default;
	[[nodiscard]] bool operator==(const str_view_t &tr) const;
	[[nodiscard]] bool operator==(const string_t &str) const;
	[[nodiscard]] bool operator==(const char_t *str) const;

	[[nodiscard]] auto operator<=>(const basic_value &other) const;
	[[nodiscard]] auto operator<=>(const str_view_t &tr) const;
	[[nodiscard]] auto operator<=>(const string_t &str) const;
	[[nodiscard]] auto operator<=>(const char_t *str) const;

public:
	basic_value &operator=(concepts::value_set<char_t> auto &&arg);
	basic_value &operator=(const basic_value&) = default;
	basic_value &operator=(basic_value&&) noexcept = default;

protected:
	string_t m_str;
};

using value    = basic_value<char    >;
using wvalue   = basic_value<wchar_t >;
using u8value  = basic_value<char8_t >;
using u16value = basic_value<char16_t>;
using u32value = basic_value<char32_t>;

template <concepts::character CharT, typename...StrArgs>
using basic_value_optl = std::optional<basic_value<CharT,StrArgs...>>;

using value_optl    = basic_value_optl<char    >;
using wvalue_optl   = basic_value_optl<wchar_t >;
using u8value_optl  = basic_value_optl<char8_t >;
using u16value_optl = basic_value_optl<char16_t>;
using u32value_optl = basic_value_optl<char32_t>;

} //namespace libgs::concepts
#include <libgs/core/detail/value.h>


#endif //LIBGS_CORE_VALUE_H
