
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

#ifndef LIBGS_DBI_DETAIL_CELL_H
#define LIBGS_DBI_DETAIL_CELL_H

#include <libgs/dbi/global.h>
#include <libgs/core/value.h>
#include <optional>

namespace libgs::dbi
{

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(string_t column_name, void *data, size_t len)
{

}

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(string_t column_name, const char *data)
{

}

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(basic_cell &&other) noexcept
{

}

template <concept_char_type CharT>
basic_cell<CharT> &basic_cell<CharT>::operator=(basic_cell &&other) noexcept
{

	return *this;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cell<CharT>::column_name() const noexcept
{

}

template <concept_char_type CharT>
bool basic_cell<CharT>::has_value() const noexcept
{

}

template <concept_char_type CharT>
bool basic_cell<CharT>::is_valid() const noexcept
{

}

template <concept_char_type CharT>
bool basic_cell<CharT>::not_has_value() const noexcept
{

}

template <concept_char_type CharT>
bool basic_cell<CharT>::is_null() const noexcept
{

}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_cell<CharT>::get(size_t base) const
{

}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_cell<CharT>::get() const
{

}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_cell<CharT>::get_or(size_t base, T default_value) const noexcept
{

}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_cell<CharT>::get_or(T default_value) const noexcept
{

}

/*
template <concept_char_type CharT>
template <typename T>
const std::basic_string<CharT> &basic_cell<CharT>::get() const noexcept 
	requires is_dsame_v<T, string_t>
{

}

template <concept_char_type CharT>
	template <typename T>
	string_t &basic_cell<CharT>::get() noexcept 
		requires is_dsame_v<T,string_t>
{

}

template <concept_char_type CharT>
	template <typename T>
	const string_t &&get() const noexcept
		requires is_dsame_v<T,string_t> &&
{

}

template <concept_char_type CharT>
	template <typename T>
	string_t &&basic_cell<CharT>::get() noexcept 
		requires is_dsame_v<T,string_t> &&
{

}

template <concept_char_type CharT>
	template <typename T>
	string_t basic_cell<CharT>::get_or() const noexcept 
		requires is_dsame_v<T,string_t>
{

}

template <concept_char_type CharT>
	const string_t &basic_cell<CharT>::get() const noexcept
{

}

template <concept_char_type CharT>
	string_t &basic_cell<CharT>::get() noexcept
{

}

template <concept_char_type CharT>
	const string_t &&basic_cell<CharT>::get() const noexcept &&
{

}

template <concept_char_type CharT>
	string_t &&basic_cell<CharT>::get() noexcept &&
{

}

template <concept_char_type CharT>
	string_t basic_cell<CharT>::get_or() const noexcept
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::to_bool() const
{

}

template <concept_char_type CharT>
	int32_t basic_cell<CharT>::to_int() const
{

}

template <concept_char_type CharT>
	uint32_t basic_cell<CharT>::to_uint() const
{

}

template <concept_char_type CharT>
	int64_t basic_cell<CharT>::to_long() const
{

}

template <concept_char_type CharT>
	uint64_t basic_cell<CharT>::to_ulong() const
{

}

template <concept_char_type CharT>
	float basic_cell<CharT>::to_float() const
{

}
	
template <concept_char_type CharT>
	double basic_cell<CharT>::to_double() const
{

}

template <concept_char_type CharT>
	long double basic_cell<CharT>::to_long_double() const
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::to_bool_or(bool default_value) const noexcept
{

}

template <concept_char_type CharT>
	int32_t basic_cell<CharT>::to_int_or(int32_t default_value) const noexcept
{

}

template <concept_char_type CharT>
	uint32_t basic_cell<CharT>::to_uint_or(uint32_t default_value) const noexcept
{

}

template <concept_char_type CharT>
	int64_t basic_cell<CharT>::to_long_or(int64_t default_value) const noexcept
{

}

template <concept_char_type CharT>
	uint64_t basic_cell<CharT>::to_ulong_or(uint64_t default_value) const noexcept
{

}

template <concept_char_type CharT>
	float basic_cell<CharT>::to_float_or(float default_value) const noexcept
{

}

template <concept_char_type CharT>
	double basic_cell<CharT>::to_double_or(double default_value) const noexcept
{

}

template <concept_char_type CharT>
	long double basic_cell<CharT>::to_long_double_or(long double default_value) const noexcept
{

}

template <concept_char_type CharT>
	const string_t &basic_cell<CharT>::to_string() const
{

}

template <concept_char_type CharT>
	string_t &basic_cell<CharT>::to_string()
{

}

template <concept_char_type CharT>
	const string_t &&basic_cell<CharT>::to_string() const &&
{

}

template <concept_char_type CharT>
	string_t &&basic_cell<CharT>::to_string() &&
{

}

template <concept_char_type CharT>
	string_t to_string_or(string_t default_value) const noexcept
{

}

template <concept_char_type CharT>
	basic_cell<CharT>::operator const string_t&() const
{

}

template <concept_char_type CharT>
	basic_cell<CharT>::operator string_t&()
{

}

template <concept_char_type CharT>
	basic_cell<CharT>::operator const string_t&&() const &&
{

}

template <concept_char_type CharT>
	basic_cell<CharT>::operator string_t&&() &&
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator==(const basic_cell &other) const noexcept
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator!=(const basic_cell &other) const noexcept
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator<(const basic_cell &other) const
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator<=(const basic_cell &other) const
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator>(const basic_cell &other) const
{

}

template <concept_char_type CharT>
	bool basic_cell<CharT>::operator>=(const basic_cell &other) const
{

}
*/

} //namespace libgs::dbi


#endif //LIBGS_DBI_DETAIL_CELL_H
