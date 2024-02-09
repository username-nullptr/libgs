
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
basic_value<CharT>::basic_value(const std::basic_string<CharT> &str) :
	base_type(str)
{

}

template <concept_char_type CharT>
basic_value<CharT>::basic_value(std::basic_string<CharT> &&str) noexcept :
	base_type(std::move(str))
{

}

template <concept_char_type CharT>
basic_value<CharT>::basic_value(libgs::basic_value<CharT> &&other) noexcept :
	base_type(std::move(other))
{

}

template <concept_char_type CharT>
basic_value<CharT>::basic_value(const CharT *str, size_type len) :
	base_type(str,len)
{

}

template <concept_char_type CharT>
template <concept_number_type T>
T basic_value<CharT>::get(size_t base) const
{
	return libgs::ston<T>(*this, base);
}

template <concept_char_type CharT>
template <typename T> requires is_dsame_v<T,std::basic_string<CharT>>
std::basic_string<CharT> &basic_value<CharT>::get()
{
	return *this;
}

template <concept_char_type CharT>
template <typename T> requires is_dsame_v<T,std::basic_string<CharT>>
const std::basic_string<CharT> &basic_value<CharT>::get() const
{
	return *this;
}

template <concept_char_type CharT>
bool basic_value<CharT>::to_bool() const
{
	return get<bool>();
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
const std::basic_string<CharT> &basic_value<CharT>::to_string() const
{
	return *this;
}

template <concept_char_type CharT>
std::basic_string<CharT> &basic_value<CharT>::to_string()
{
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::set_value(const std::basic_string<CharT> &v)
{
	operator=(v);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::set_value(std::basic_string<CharT> &&v)
{
	operator=(std::move(v));
	return *this;
}

template <concept_char_type CharT>
template <typename Arg0, typename...Args>
basic_value<CharT> &basic_value<CharT>::set_value(format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args)
{
	operator=(std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...));
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> basic_value<CharT>::from(const std::basic_string<CharT> &v)
{
	return this_type(v);
}

template <concept_char_type CharT>
basic_value<CharT> basic_value<CharT>::from(std::basic_string<CharT> &&v)
{
	return this_type(std::move(v));
}

template <concept_char_type CharT>
template <typename Arg0, typename...Args>
basic_value<CharT> basic_value<CharT>::from(format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args)
{
	value hv;
	hv.set_value(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
	return hv;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(const std::basic_string<CharT> &other)
{
	base_type::operator=(other);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(const basic_value<CharT> &other)
{
	base_type::operator=(other);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(std::basic_string<CharT> &&other) noexcept
{
	base_type::operator=(std::move(other));
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(basic_value<CharT> &&other) noexcept
{
	base_type::operator=(std::move(other));
	return *this;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_VALUE_H
