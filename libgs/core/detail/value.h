
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

namespace libgs
{

template <concepts::char_type CharT>
basic_value<CharT>::basic_value(concepts::basic_value_arg<char_t> auto &&arg)
{
	set(std::forward<decltype(arg)>(arg));
}

template <concepts::char_type CharT>
template <typename Arg0, typename...Args>
basic_value<CharT>::basic_value(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args) :
	basic_value(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <concepts::char_type CharT>
std::basic_string<CharT> &basic_value<CharT>::to_string() & noexcept
{
	return get<string_t>();
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &basic_value<CharT>::to_string() const & noexcept
{
	return get<string_t>();
}

template <concepts::char_type CharT>
std::basic_string<CharT> &&basic_value<CharT>::to_string() && noexcept
{
	return std::move(get<string_t>());
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &&basic_value<CharT>::to_string() const && noexcept
{
	return std::move(get<string_t>());
}

template <concepts::char_type CharT>
basic_value<CharT>::operator std::basic_string<CharT>&() & noexcept
{
	return to_string();
}

template <concepts::char_type CharT>
basic_value<CharT>::operator const std::basic_string<CharT>&() const & noexcept
{
	return to_string();
}

template <concepts::char_type CharT>
basic_value<CharT>::operator std::basic_string<CharT>&&() && noexcept
{
	return std::move(to_string());
}

template <concepts::char_type CharT>
basic_value<CharT>::operator const std::basic_string<CharT>&&() const && noexcept
{
	return std::move(to_string());
}

template <concepts::char_type CharT>
template <concepts::integral_type T>
T basic_value<CharT>::get(size_t base) const
{
	return libgs::ston<T>(m_str, base);
}

template <concepts::char_type CharT>
template <concepts::float_type T>
T basic_value<CharT>::get() const
{
	return libgs::ston<T>(m_str);
}

template <concepts::char_type CharT>
template <concepts::integral_type T>
T basic_value<CharT>::get_or(size_t base, T def_value) const noexcept
{
	return libgs::ston_or<T>(m_str, base, def_value);
}

template <concepts::char_type CharT>
template <concepts::float_type T>
T basic_value<CharT>::get_or(T def_value) const noexcept
{
	return libgs::ston_or<T>(m_str, def_value);
}

template <concepts::char_type CharT>
template <concepts::basic_vgs<CharT> T>
decltype(auto) basic_value<CharT>::get() & noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return return_reference(m_str);
	else if constexpr( std::is_same_v<T,str_view_t> )
		return str_view_t(m_str);
	else
		return return_reference(*this);
}

template <concepts::char_type CharT>
template <concepts::basic_rvgs<CharT> T>
decltype(auto) basic_value<CharT>::get() && noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return std::move(m_str);
	else
		return std::move(*this);
}

template <concepts::char_type CharT>
template <concepts::basic_vgs<CharT> T>
auto &&basic_value<CharT>::get() const & noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return return_reference(m_str);
	else if constexpr( std::is_same_v<T,str_view_t> )
		return str_view_t(m_str);
	else
		return return_reference(*this);
}

template <concepts::char_type CharT>
template <concepts::basic_rvgs<CharT> T>
auto &&basic_value<CharT>::get() const && noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return std::move(m_str);
	else
		return std::move(*this);
}

template <concepts::char_type CharT>
std::basic_string<CharT> &basic_value<CharT>::get() & noexcept
{
	return m_str;
}

template <concepts::char_type CharT>
std::basic_string<CharT> &&basic_value<CharT>::get() && noexcept
{
	return std::move(m_str);
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &basic_value<CharT>::get() const & noexcept
{
	return m_str;
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &&basic_value<CharT>::get() const && noexcept
{
	return std::move(m_str);
}

template <concepts::char_type CharT>
bool basic_value<CharT>::to_bool(size_t base) const
{
	return get_or<bool>(base);
}

template <concepts::char_type CharT>
int32_t basic_value<CharT>::to_int(size_t base) const
{
	return get<int32_t>(base);
}

template <concepts::char_type CharT>
uint32_t basic_value<CharT>::to_uint(size_t base) const
{
	return get<uint32_t>(base);
}

template <concepts::char_type CharT>
int64_t basic_value<CharT>::to_long(size_t base) const
{
	return get<int64_t>(base);
}

template <concepts::char_type CharT>
uint64_t basic_value<CharT>::to_ulong(size_t base) const
{
	return get<uint64_t>(base);
}

template <concepts::char_type CharT>
float basic_value<CharT>::to_float() const
{
	return get<float>();
}

template <concepts::char_type CharT>
double basic_value<CharT>::to_double() const
{
	return get<double>();
}

template <concepts::char_type CharT>
long double basic_value<CharT>::to_ldouble() const
{
	return get<long double>();
}

template <concepts::char_type CharT>
bool basic_value<CharT>::to_bool_or(size_t base, bool def_value) const noexcept
{
	return get_or<bool>(base, def_value);
}

template <concepts::char_type CharT>
int32_t basic_value<CharT>::to_int_or(size_t base, int32_t def_value) const noexcept
{
	return get_or<int32_t>(base, def_value);
}

template <concepts::char_type CharT>
uint32_t basic_value<CharT>::to_uint_or(size_t base, uint32_t def_value) const noexcept
{
	return get_or<uint32_t>(base, def_value);
}

template <concepts::char_type CharT>
int64_t basic_value<CharT>::to_long_or(size_t base, int64_t def_value) const noexcept
{
	return get_or<int64_t>(base, def_value);
}

template <concepts::char_type CharT>
uint64_t basic_value<CharT>::to_ulong_or(size_t base, uint64_t def_value) const noexcept
{
	return get_or<uint64_t>(base, def_value);
}

template <concepts::char_type CharT>
float basic_value<CharT>::to_float_or(float def_value) const noexcept
{
	return get_or<float>(def_value);
}

template <concepts::char_type CharT>
double basic_value<CharT>::to_double_or(double def_value) const noexcept
{
	return get_or<double>(def_value);
}

template <concepts::char_type CharT>
long double basic_value<CharT>::to_ldouble_or(long double def_value) const noexcept
{
	return get_or<long double>(def_value);
}

template <concepts::char_type CharT>
template <typename Arg0, typename...Args>
void basic_value<CharT>::set(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args)
{
	m_str = std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
}

template <concepts::char_type CharT>
void basic_value<CharT>::set(concepts::basic_value_arg<char_t> auto &&arg)
{
	using Arg = decltype(arg);
	using arg_t = std::remove_cvref_t<Arg>;

	if constexpr( std::is_same_v<arg_t, std::basic_string_view<CharT>> )
		m_str = string_t(arg.data(), arg.size());
	else if constexpr( is_basic_string_v<Arg, CharT> )
		m_str = std::forward<Arg>(arg);
	else
		m_str = std::format(default_format_v<CharT>, std::forward<Arg>(arg));
}

template <concepts::char_type CharT>
bool basic_value<CharT>::is_alpha() const noexcept
{
	for(auto &c : m_str)
	{
		if( not std::isalpha(c) )
			return false;
	}
	return true;
}

template <concepts::char_type CharT>
bool basic_value<CharT>::is_digit() const noexcept
{
	for(auto &c : m_str)
	{
		if( not std::isdigit(c) )
			return false;
	}
	return true;
}

template <concepts::char_type CharT>
bool basic_value<CharT>::is_alnum() const noexcept
{
	for(auto &c : m_str)
	{
		if( not std::isalnum(c) )
			return false;
	}
	return true;
}

template <concepts::char_type CharT>
std::basic_string<CharT> &basic_value<CharT>::operator*() & noexcept
{
	return get();
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &basic_value<CharT>::operator*() const & noexcept
{
	return get();
}

template <concepts::char_type CharT>
std::basic_string<CharT> &&basic_value<CharT>::operator*() && noexcept
{
	return std::move(get());
}

template <concepts::char_type CharT>
const std::basic_string<CharT> &&basic_value<CharT>::operator*() const && noexcept
{
	return std::move(get());
}

template <concepts::char_type CharT>
std::basic_string<CharT> *basic_value<CharT>::operator->() noexcept
{
	return &get();
}

template <concepts::char_type CharT>
const std::basic_string<CharT> *basic_value<CharT>::operator->() const noexcept
{
	return &get();
}

template <concepts::char_type CharT>
bool basic_value<CharT>::operator==(const str_view_t &str) const
{
	return m_str == str;
}

template <concepts::char_type CharT>
bool basic_value<CharT>::operator==(const string_t &str) const
{
	return m_str == str;
}

template <concepts::char_type CharT>
auto basic_value<CharT>::operator<=>(const basic_value &other) const
{
	return m_str <=> other.to_string();
}

template <concepts::char_type CharT>
auto basic_value<CharT>::operator<=>(const str_view_t &str) const
{
	return m_str <=> str;
}

template <concepts::char_type CharT>
auto basic_value<CharT>::operator<=>(const string_t &str) const
{
	return m_str <=> str;
}

template <concepts::char_type CharT>
basic_value<CharT> &basic_value<CharT>::operator=(concepts::basic_value_arg<char_t> auto &&arg)
{
	set(std::forward<decltype(arg)>(arg));
	return *this;
}

} //namespace libgs

namespace std
{

template <libgs::concepts::char_type CharT>
struct hash<libgs::basic_value<CharT>>
{
	size_t operator()(const libgs::basic_value<CharT> &v) const noexcept {
		return hash<std::basic_string<CharT>>()(v);
	}
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<libgs::basic_value<CharT>, CharT>
{
	auto format(const libgs::basic_value<CharT> &value, auto &context) const {
		return m_formatter.format(value.to_string(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};

} //namespace std


#endif //LIBGS_CORE_DETAIL_VALUE_H
