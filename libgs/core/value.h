
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

namespace libgs
{

template <concept_char_type CharT>
class basic_value : public std::basic_string<CharT>
{
	template <typename...Args>
	using format_string = libgs::format_string<CharT, Args...>;

	static constexpr const CharT *default_format_v = libgs::default_format_v<CharT>;

public:
	using base_type = std::basic_string<CharT>;
	using this_type = basic_value<CharT>;
	using size_type = base_type::size_type;

public:
	template <typename CT>
	struct is_string
	{
		static constexpr bool value =
			is_dsame_v<CT, basic_value<char>> or
			is_dsame_v<CT, basic_value<wchar_t>> or
			libgs::is_string_v<CT>;
	};

	template <typename CT>
	static constexpr bool is_string_v = is_string<CT>::value;

public:
	basic_value() = default;
	basic_value(const std::basic_string<CharT> &str);
	basic_value(const libgs::basic_value<CharT> &other) = default;
	basic_value(std::basic_string<CharT> &&str) noexcept;
	basic_value(libgs::basic_value<CharT> &&other) noexcept;
	basic_value(const CharT *str, size_type len);

	template <typename T> requires (not is_string_v<T>)
	basic_value(T &&v) {
		operator=(std::format(default_format_v, std::forward<T>(v)));
	}

public:
	template <concept_number_type T>
	[[nodiscard]] T get(size_t base = 10) const;

	template <typename T> requires is_dsame_v<T,std::basic_string<CharT>>
	[[nodiscard]] const std::basic_string<CharT> &get() const;

	template <typename T> requires is_dsame_v<T,std::basic_string<CharT>>
	[[nodiscard]] std::basic_string<CharT> &get();

public:
	[[nodiscard]] bool to_bool() const;
	[[nodiscard]] int32_t to_int(size_t base = 10) const;
	[[nodiscard]] uint32_t to_uint(size_t base = 10) const;
	[[nodiscard]] int64_t to_long(size_t base = 10) const;
	[[nodiscard]] uint64_t to_ulong(size_t base = 10) const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_ldouble() const;

public:
	[[nodiscard]] const std::basic_string<CharT> &to_string() const;
	[[nodiscard]] std::basic_string<CharT> &to_string();

public:
	this_type &set_value(const std::basic_string<CharT> &v);
	this_type &set_value(std::basic_string<CharT> &&v);

	template <typename Arg0, typename...Args>
	this_type &set_value(format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

	template <typename T> requires (not is_string_v<T>)
	this_type &set_value(T &&v) {
		return set_value(default_format_v, std::forward<T>(v));
	}

public:
	static this_type from(const std::basic_string<CharT> &v);
	static this_type from(std::basic_string<CharT> &&v);

	template <typename Arg0, typename...Args>
	[[nodiscard]] static this_type from(format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

	template <typename T> requires (not is_string_v<T>)
	[[nodiscard]] static this_type from(T &&v)
	{
		this_type hv;
		hv.set_value(default_format_v, std::forward<T>(v));
		return hv;
	}

public:
	template <typename T> requires (not is_string_v<T>)
	this_type &operator=(T &&value)
	{
		set_value(std::forward<T>(value));
		return *this;
	}

public:
	this_type &operator=(const std::basic_string<CharT> &other);
	this_type &operator=(const libgs::basic_value<CharT> &other);
	this_type &operator=(std::basic_string<CharT> &&other) noexcept;
	this_type &operator=(libgs::basic_value<CharT> &&other) noexcept;
};

using value = basic_value<char>;
using wvalue = basic_value<wchar_t>;

template <concept_char_type CharT>
using basic_value_list = std::deque<libgs::basic_value<CharT>>;

using value_list = basic_value_list<char>;
using wvalue_list = basic_value_list<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/value.h>


#endif //LIBGS_CORE_VALUE_H
