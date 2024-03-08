
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

#ifndef LIBGS_CORE_DETAIL_VALUE_H
#define LIBGS_CORE_DETAIL_VALUE_H

namespace std
{

template <libgs::concept_char_type CharT>
struct hash<libgs::basic_value<CharT>>
{
	size_t operator()(const libgs::basic_value<CharT> &v) const noexcept {
		return hash<std::basic_string<CharT>>()(v);
	}
};

} //namespace std

namespace libgs
{

template <concept_char_type CharT>
basic_value<CharT>::basic_value(const CharT *str) :
	m_str(str)
{

}

template <concept_char_type CharT>
basic_value<CharT>::basic_value(str_type str) :
	m_str(std::move(str))
{

}

template <concept_char_type CharT>
basic_value<CharT>::basic_value(str_view_type str) :
	m_str(str.data(), str.size())
{

}

template <concept_char_type CharT>
template <typename Arg0, typename...Args>
basic_value<CharT>::basic_value(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args) :
	basic_value<CharT>(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <concept_char_type CharT>
template <typename T>
basic_value<CharT>::basic_value(T &&v) requires (
	not requires(T &&rv) { str_type(std::forward<T>(rv)); }
) : basic_value<CharT>(default_format_v, std::forward<T>(v))
{

}

template <concept_char_type CharT>
std::basic_string<CharT> &basic_value<CharT>::to_string() noexcept
{
	return get<str_type>();
}

template <concept_char_type CharT>
const std::basic_string<CharT> &basic_value<CharT>::to_string() const noexcept
{
	return get<str_type>();
}

template <concept_char_type CharT>
basic_value<CharT>::operator std::basic_string<CharT>&()
{
	return to_string();
}

template <concept_char_type CharT>
basic_value<CharT>::operator const std::basic_string<CharT>&() const
{
	return to_string();
}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_value<CharT>::get(size_t base) const
{
	return libgs::ston<T>(m_str, base);
}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_value<CharT>::get() const
{
	return libgs::ston<T>(m_str);
}

template <concept_char_type CharT>
template <concept_integral_type T>
T basic_value<CharT>::get_or(size_t base, T default_value) const noexcept
{
	return libgs::ston<T>(m_str, base, default_value);
}

template <concept_char_type CharT>
template <concept_float_type T>
T basic_value<CharT>::get_or(T default_value) const noexcept
{
	return libgs::ston<T>(m_str, default_value);
}

template <concept_char_type CharT>
template <typename T>
const std::basic_string<CharT> &basic_value<CharT>::get() const noexcept requires
	is_dsame_v<T,std::basic_string<CharT>>
{
	return m_str;
}

template <concept_char_type CharT>
template <typename T>
std::basic_string<CharT> &basic_value<CharT>::get() noexcept requires
	is_dsame_v<T,std::basic_string<CharT>>
{
	return m_str;
}

template <concept_char_type CharT>
bool basic_value<CharT>::to_bool(size_t base) const
{
	return get_or<bool>(base);
}

template <concept_char_type CharT>
int32_t basic_value<CharT>::to_int(size_t base) const
{
	return get<int32_t>(base);
}

template <concept_char_type CharT>
uint32_t basic_value<CharT>::to_uint(size_t base) const
{
	return get<uint32_t>(base);
}

template <concept_char_type CharT>
int64_t basic_value<CharT>::to_long(size_t base) const
{
	return get<int64_t>(base);
}

template <concept_char_type CharT>
uint64_t basic_value<CharT>::to_ulong(size_t base) const
{
	return get<uint64_t>(base);
}

template <concept_char_type CharT>
float basic_value<CharT>::to_float() const
{
	return get<float>();
}

template <concept_char_type CharT>
double basic_value<CharT>::to_double() const
{
	return get<double>();
}

template <concept_char_type CharT>
long double basic_value<CharT>::to_ldouble() const
{
	return get<long double>();
}

template <concept_char_type CharT>
bool basic_value<CharT>::to_bool_or(size_t base, bool default_value) const noexcept
{
	return get_or<bool>(base, default_value);
}

template <concept_char_type CharT>
int32_t basic_value<CharT>::to_int_or(size_t base, int32_t default_value) const noexcept
{
	return get_or<int32_t>(base, default_value);
}

template <concept_char_type CharT>
uint32_t basic_value<CharT>::to_uint_or(size_t base, uint32_t default_value) const noexcept
{
	return get_or<uint32_t>(base, default_value);
}

template <concept_char_type CharT>
int64_t basic_value<CharT>::to_long_or(size_t base, int64_t default_value) const noexcept
{
	return get_or<int64_t>(base, default_value);
}

template <concept_char_type CharT>
uint64_t basic_value<CharT>::to_ulong_or(size_t base, uint64_t default_value) const noexcept
{
	return get_or<uint64_t>(base, default_value);
}

template <concept_char_type CharT>
float basic_value<CharT>::to_float_or(float default_value) const noexcept
{
	return get_or<float>(default_value);
}

template <concept_char_type CharT>
double basic_value<CharT>::to_double_or(double default_value) const noexcept
{
	return get_or<double>(default_value);
}

template <concept_char_type CharT>
long double basic_value<CharT>::to_ldouble_or(long double default_value) const noexcept
{
	return get_or<long double>(default_value);
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::set(const CharT *str)
{
	m_str = str;
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::set(str_type str)
{
	m_str = std::move(str);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::set(str_view_type str)
{
	m_str = str_type(str.data(), str.size());
	return *this;
}

template <concept_char_type CharT>
template <typename Arg0, typename...Args>
basic_value<CharT> &basic_value<CharT>::set(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args)
{
	m_str = std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_value<CharT> &basic_value<CharT>::set(T &&v) requires (
	not requires(T &&rv) { str_type(std::forward<T>(rv)); } )
{
	return set(default_format_v, v);
}

template <concept_char_type CharT>
bool basic_value<CharT>::operator<=>(const basic_value &other) const
{
	return m_str <=> other.m_str;
}

template <concept_char_type CharT>
bool basic_value<CharT>::operator<=>(const str_view_type &str) const
{
	return m_str <=> str;
}

template <concept_char_type CharT>
bool basic_value<CharT>::operator<=>(const str_type &str) const
{
	return m_str <=> str;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(const CharT *str)
{
	return set(str);
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(str_type str)
{
	return set(std::move(str));
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(str_view_type str)
{
	return set(std::move(str));
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_VALUE_H
