
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

#ifndef LIBGS_DBI_CELL_H
#define LIBGS_DBI_CELL_H

#include <libgs/dbi/global.h>
#include <libgs/core/value.h>
#include <optional>

namespace libgs::dbi
{

template <concept_char_type CharT>
struct default_column_name;

template <>
struct default_column_name<char> {
	static constexpr const char *value = "NULL";
};

template <>
struct default_column_name<wchar_t> {
	static constexpr const wchar_t *value = L"NULL";
};

template <typename T>
concept concept_cgb =
	not concept_number_type<T> and
	not is_wchar_string_v<T> and
	not is_char_string_v<T>;

template <concept_char_type CharT>
class LIBGS_DBI_API basic_cell
{
public:
	using string_t = std::basic_string<CharT>;

public:
	basic_cell() = default;
	basic_cell(string_t column_name, void *data, size_t len);
	basic_cell(string_t column_name, const char *data);

	basic_cell(const basic_cell &other) = default;
	basic_cell(basic_cell &&other) noexcept;

	basic_cell &operator=(const basic_cell &other) = default;
	basic_cell &operator=(basic_cell &&other) noexcept;

public:
	[[nodiscard]] string_t column_name() const noexcept;
	[[nodiscard]] bool has_value() const noexcept;
	[[nodiscard]] bool is_valid() const noexcept;
	[[nodiscard]] bool not_has_value() const noexcept;
	[[nodiscard]] bool is_null() const noexcept;

public:
	template <concept_integral_type T>
	[[nodiscard]] T get(size_t base = 10) const;

	template <concept_float_type T>
	[[nodiscard]] T get() const;

	template <concept_integral_type T>
	[[nodiscard]] T get_or(size_t base = 10, T default_value = 0) const noexcept;

	template <concept_float_type T>
	[[nodiscard]] T get_or(T default_value = 0.0) const noexcept;

	template <concept_vgs<CharT> T>
	[[nodiscard]] string_t get() const noexcept;

	template <concept_vgs<CharT> T>
	[[nodiscard]] string_t get_or() const noexcept;

	template <concept_cgb T>
	[[nodiscard]] T get();

	template <concept_cgb T>
	[[nodiscard]] T get_or(T default_value = {});

public:
	[[nodiscard]] bool to_bool() const;
	[[nodiscard]] int32_t to_int() const;
	[[nodiscard]] uint32_t to_uint() const;
	[[nodiscard]] int64_t to_long() const;
	[[nodiscard]] uint64_t to_ulong() const;
	[[nodiscard]] float to_float() const;
	[[nodiscard]] double to_double() const;
	[[nodiscard]] long double to_long_double() const;
	[[nodiscard]] string_t to_string() const;

public:
	[[nodiscard]] bool to_bool_or(bool default_value = false) const noexcept;
	[[nodiscard]] int32_t to_int_or(int32_t default_value = 0) const noexcept;
	[[nodiscard]] uint32_t to_uint_or(uint32_t default_value = 0) const noexcept;
	[[nodiscard]] int64_t to_long_or(int64_t default_value = 0) const noexcept;
	[[nodiscard]] uint64_t to_ulong_or(uint64_t default_value = 0) const noexcept;
	[[nodiscard]] float to_float_or(float default_value = 0.0) const noexcept;
	[[nodiscard]] double to_double_or(double default_value = 0.0) const noexcept;
	[[nodiscard]] long double to_long_double_or(long double default_value = 0.0) const noexcept;
	[[nodiscard]] string_t to_string_or(string_t default_value = {}) const noexcept;

public:
	[[nodiscard]] std::string &blob() & noexcept;
	[[nodiscard]] const std::string &blob() const & noexcept;
	[[nodiscard]] std::string &&blob() && noexcept;
	[[nodiscard]] const std::string &&blob() const && noexcept;

public:
	[[nodiscard]] bool operator==(const basic_cell &other) const noexcept;
	[[nodiscard]] bool operator!=(const basic_cell &other) const noexcept;
	[[nodiscard]] bool operator<(const basic_cell &other) const;
	[[nodiscard]] bool operator<=(const basic_cell &other) const;
	[[nodiscard]] bool operator>(const basic_cell &other) const;
	[[nodiscard]] bool operator>=(const basic_cell &other) const;

private:
	string_t m_column_name = default_column_name<CharT>::value;
	std::optional<value> m_data;
};

using cell = basic_cell<char>;
using wcell = basic_cell<wchar_t>;

} //namespace libgs::dbi
#include <libgs/dbi/detail/cell.h>


#endif //LIBGS_DBI_CELL_H
