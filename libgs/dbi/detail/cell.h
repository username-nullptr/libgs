
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
basic_cell<CharT>::basic_cell(string_view_t column_name) :
	m_column_name(column_name.data(), column_name.size())
{
	if( m_column_name.empty() )
		m_column_name = default_column_name<CharT>::value;
}

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(string_view_t column_name, void *data, size_t len) :
	m_column_name(column_name.data(), column_name.size()),
	m_data(std::string(reinterpret_cast<char*>(data), len))
{
	if( m_column_name.empty() )
		m_column_name = default_column_name<CharT>::value;
}

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(string_view_t column_name, const char *data) :
	m_column_name(column_name.data(), column_name.size()),
	m_data(data)
{
	if( m_column_name.empty() )
		m_column_name = default_column_name<CharT>::value;
}

template <concept_char_type CharT>
basic_cell<CharT>::basic_cell(basic_cell &&other) noexcept :
	m_column_name(std::move(other.m_column_name)),
	m_data(std::move(other.m_data))
{
	other.m_column_name = default_column_name<CharT>::value;
}

template <concept_char_type CharT>
basic_cell<CharT> &basic_cell<CharT>::operator=(basic_cell &&other) noexcept
{
	m_column_name = std::move(other.m_column_name);
	other.m_column_name = default_column_name<CharT>::value;
	m_data = std::move(other.m_data);
	return *this;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_cell<CharT>::column_name() const noexcept
{
	return m_column_name;
}

template <concept_char_type CharT>
bool basic_cell<CharT>::has_value() const noexcept
{
	return m_data.has_value();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::is_valid() const noexcept
{
	return m_data.has_value();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::not_has_value() const noexcept
{
	return not has_value();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::is_null() const noexcept
{
	return not_has_value();
}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_cell<CharT>::get(size_t base) const
{
	if( has_value() )
		return m_data.value().template get<T>(base);
	throw runtime_error("libgs::dbi::cell::get: not has value.");
	// return 0;
}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_cell<CharT>::get() const
{
	if( has_value() )
		return m_data.value().template get<T>();
	throw runtime_error("libgs::dbi::cell::get: not has value.");
	// return 0.0;
}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_cell<CharT>::get_or(size_t base, T default_value) const noexcept
{
	if( has_value() )
		return m_data.value().template get<T>(base);
	return default_value;
}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_cell<CharT>::get_or(T default_value) const noexcept
{
	if( has_value() )
		return m_data.value().template get<T>();
	return default_value;
}

template <concept_char_type CharT>
template <concept_vgs<CharT> T>
std::basic_string<CharT> basic_cell<CharT>::get() const
{
	if( not has_value() )
		throw runtime_error("libgs::dbi::cell::get: not has value.");

	if constexpr ( is_char_v<CharT> )
		return m_data.value();
	else
		return mbstowcs(m_data.value().get());
}

template <concept_char_type CharT>
template <concept_vgs<CharT> T>
std::basic_string<CharT> basic_cell<CharT>::get_or(string_view_t default_value) const noexcept
{
	if( not has_value() )
		return {default_value.data(), default_value.size()};

	if constexpr ( is_char_v<CharT> )
		return m_data.value();
	else
		return mbstowcs(m_data.value().get());
}

template <concept_char_type CharT>
template <concept_cgb T>
T basic_cell<CharT>::get() const
{
	if( not has_value() )
		throw runtime_error("libgs::dbi::cell::get: not has value.");
	else if( m_data.value()->size() != sizeof(T) )
		throw runtime_error("libgs::dbi::cell::get: memory_size != sizeof(T).");
	return *reinterpret_cast<T*>(m_data.value().get().c_str());
}

template <concept_char_type CharT>
template <concept_cgb T>
T basic_cell<CharT>::get_or(T default_value) const
{
	if( has_value() and m_data.value()->size() == sizeof(T) )
		return *reinterpret_cast<T*>(m_data.value().get().c_str());
	return default_value;
}

template <concept_char_type CharT>
bool basic_cell<CharT>::to_bool(size_t base) const
{
	return get<bool>(base);
}

template <concept_char_type CharT>
int32_t basic_cell<CharT>::to_int(size_t base) const
{
	return get<int32_t>(base);
}

template <concept_char_type CharT>
uint32_t basic_cell<CharT>::to_uint(size_t base) const
{
	return get<uint32_t>(base);
}

template <concept_char_type CharT>
int64_t basic_cell<CharT>::to_long(size_t base) const
{
	return get<int64_t>(base);
}

template <concept_char_type CharT>
uint64_t basic_cell<CharT>::to_ulong(size_t base) const
{
	return get<uint64_t>(base);
}

template <concept_char_type CharT>
float basic_cell<CharT>::to_float() const
{
	return get<float>();
}
	
template <concept_char_type CharT>
double basic_cell<CharT>::to_double() const
{
	return get<double>();
}

template <concept_char_type CharT>
long double basic_cell<CharT>::to_long_double() const
{
	return get<long double>();
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cell<CharT>::to_string() const
{
	return get<string_t>();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::to_bool_or(size_t base, bool default_value) const noexcept
{
	return get_or<bool>(base, default_value);
}

template <concept_char_type CharT>
int32_t basic_cell<CharT>::to_int_or(size_t base, int32_t default_value) const noexcept
{
	return get_or<int32_t>(base, default_value);
}

template <concept_char_type CharT>
uint32_t basic_cell<CharT>::to_uint_or(size_t base, uint32_t default_value) const noexcept
{
	return get_or<uint32_t>(base, default_value);
}

template <concept_char_type CharT>
int64_t basic_cell<CharT>::to_long_or(size_t base, int64_t default_value) const noexcept
{
	return get_or<int64_t>(base, default_value);
}

template <concept_char_type CharT>
uint64_t basic_cell<CharT>::to_ulong_or(size_t base, uint64_t default_value) const noexcept
{
	return get_or<uint64_t>(base, default_value);
}

template <concept_char_type CharT>
float basic_cell<CharT>::to_float_or(float default_value) const noexcept
{
	return get_or<float>(default_value);
}

template <concept_char_type CharT>
double basic_cell<CharT>::to_double_or(double default_value) const noexcept
{
	return get_or<double>(default_value);
}

template <concept_char_type CharT>
long double basic_cell<CharT>::to_long_double_or(long double default_value) const noexcept
{
	return get_or<long double>(default_value);
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_cell<CharT>::to_string_or(string_view_t default_value) const noexcept
{
	return get_or<string_t>(default_value);
}

template <concept_char_type CharT>
std::string &basic_cell<CharT>::blob() &
{
	if( has_value() )
		return m_data.value().get();
	throw runtime_error("libgs::dbi::cell::get: not has value.");
}

template <concept_char_type CharT>
const std::string &basic_cell<CharT>::blob() const &
{
	if( has_value() )
		return m_data.value().get();
	throw runtime_error("libgs::dbi::cell::get: not has value.");
}

template <concept_char_type CharT>
std::string &&basic_cell<CharT>::blob() &&
{
	if( has_value() )
		return std::move(m_data.value().get());
	throw runtime_error("libgs::dbi::cell::get: not has value.");
}

template <concept_char_type CharT>
const std::string &&basic_cell<CharT>::blob() const &&
{
	if( has_value() )
		return std::move(m_data.value().get());
	throw runtime_error("libgs::dbi::cell::get: not has value.");
}

template <concept_char_type CharT>
std::string basic_cell<CharT>::blob_or(std::string_view default_value) const noexcept
{
	if( has_value() )
		return m_data.value().get();
	return {default_value.data(), default_value.size()};
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator==(const basic_cell &other) const noexcept
{
	return has_value() and other.has_value() and m_data.value() == other.m_data.value();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator!=(const basic_cell &other) const noexcept
{
	return not operator==(other);
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator<(const basic_cell &other) const
{
	return get() < other.get();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator<=(const basic_cell &other) const
{
	return get() <= other.get();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator>(const basic_cell &other) const
{
	return get() > other.get();
}

template <concept_char_type CharT>
bool basic_cell<CharT>::operator>=(const basic_cell &other) const
{
	return get() >= other.get();
}

} //namespace libgs::dbi

inline bool operator==(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.has_value() and cell.to_string() == str;
}

inline bool operator!=(const libgs::dbi::cell &cell, const std::string &str)
{
	return not operator==(cell, str);
}

inline bool operator>(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.to_string() > str;
}

inline bool operator<(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.to_string() < str;
}

inline bool operator>=(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.to_string() >= str;
}

inline bool operator<=(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.to_string() <= str;
}

inline bool operator==(const std::string &str, const libgs::dbi::cell &cell)
{
	return cell.has_value() and cell.to_string() == str;
}

inline bool operator!=(const std::string &str, const libgs::dbi::cell &cell)
{
	return not operator==(cell, str);
}

inline bool operator>(const std::string &str, const libgs::dbi::cell &cell)
{
	return str > cell.to_string();
}

inline bool operator<(const std::string &str, const libgs::dbi::cell &cell)
{
	return str < cell.to_string();
}

inline bool operator>=(const std::string &str, const libgs::dbi::cell &cell)
{
	return str >= cell.to_string();
}

inline bool operator<=(const std::string &str, const libgs::dbi::cell &cell)
{
	return str <= cell.to_string();
}

inline std::string operator+(const libgs::dbi::cell &cell, const std::string &str)
{
	return cell.has_value() ? cell.to_string() + str : str;
}

inline std::string operator+(const std::string &str, const libgs::dbi::cell &cell)
{
	return cell.has_value() ? str + cell.to_string() : str;
}

inline bool operator==(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.has_value() and cell.to_string() == str;
}

inline bool operator!=(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return not operator==(cell, str);
}

inline bool operator>(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.to_string() > str;
}

inline bool operator<(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.to_string() < str;
}

inline bool operator>=(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.to_string() >= str;
}

inline bool operator<=(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.to_string() <= str;
}

inline bool operator==(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return cell.has_value() and cell.to_string() == str;
}

inline bool operator!=(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return not operator==(cell, str);
}

inline bool operator>(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return str > cell.to_string();
}

inline bool operator<(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return str < cell.to_string();
}

inline bool operator>=(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return str >= cell.to_string();
}

inline bool operator<=(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return str <= cell.to_string();
}

inline std::wstring operator+(const libgs::dbi::wcell &cell, const std::wstring &str)
{
	return cell.has_value() ? cell.to_string() + str : str;
}

inline std::wstring operator+(const std::wstring &str, const libgs::dbi::wcell &cell)
{
	return cell.has_value() ? str + cell.to_string() : str;
}


#endif //LIBGS_DBI_DETAIL_CELL_H
