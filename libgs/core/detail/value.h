
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

template <concepts::character CharT, typename Traits, class Alloc>
basic_value<CharT,Traits,Alloc>::basic_value(concepts::value_arg_p<char_t> auto &&arg)
{
	set(std::forward<decltype(arg)>(arg));
}

template <concepts::character CharT, typename Traits, class Alloc>
template <typename Arg0, typename...Args>
basic_value<CharT,Traits,Alloc>::basic_value
(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args) :
	basic_value(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::to_string() & noexcept
{
	return get<string_t>();
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::to_string() const & noexcept
{
	return get<string_t>();
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::to_string() && noexcept
{
	return std::move(get<string_t>());
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::to_string() const && noexcept
{
	return std::move(get<string_t>());
}

template <concepts::character CharT, typename Traits, typename Alloc>
basic_value<CharT,Traits,Alloc>::operator string_t&() & noexcept
{
	return to_string();
}

template <concepts::character CharT, typename Traits, typename Alloc>
basic_value<CharT,Traits,Alloc>::operator const string_t&() const & noexcept
{
	return to_string();
}

template <concepts::character CharT, typename Traits, typename Alloc>
basic_value<CharT,Traits,Alloc>::operator string_t&&() && noexcept
{
	return std::move(to_string());
}

template <concepts::character CharT, typename Traits, typename Alloc>
basic_value<CharT,Traits,Alloc>::operator const string_t&&() const && noexcept
{
	return std::move(to_string());
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::integral_p T>
T basic_value<CharT,Traits,Alloc>::get(size_t base) const
{
	return strtls::to_arith<T>(m_str, base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::floating_p T>
T basic_value<CharT,Traits,Alloc>::get() const
{
	return strtls::to_arith<T>(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::integral_p T>
T basic_value<CharT,Traits,Alloc>::get_or(size_t base, T def_value) const noexcept
{
	return strtls::to_arith_or<T>(m_str, base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::floating_p T>
T basic_value<CharT,Traits,Alloc>::get_or(T def_value) const noexcept
{
	return strtls::to_arith_or<T>(m_str, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::vgs<CharT> T>
decltype(auto) basic_value<CharT,Traits,Alloc>::get() & noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return return_reference(m_str);
	else if constexpr( std::is_same_v<T,str_view_t> )
		return str_view_t(m_str);
	else
		return return_reference(*this);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::rvgs<CharT> T>
decltype(auto) basic_value<CharT,Traits,Alloc>::get() && noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return std::move(m_str);
	else
		return std::move(*this);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::vgs<CharT> T>
auto &&basic_value<CharT,Traits,Alloc>::get() const & noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return return_reference(m_str);
	else if constexpr( std::is_same_v<T,str_view_t> )
		return str_view_t(m_str);
	else
		return return_reference(*this);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <concepts::rvgs<CharT> T>
auto &&basic_value<CharT,Traits,Alloc>::get() const && noexcept
{
	if constexpr( std::is_same_v<T,string_t> )
		return std::move(m_str);
	else
		return std::move(*this);
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::get() & noexcept
{
	return m_str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::get() && noexcept
{
	return std::move(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::get() const & noexcept
{
	return m_str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::get() const && noexcept
{
	return std::move(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::to_bool(size_t base) const
{
	return get_or<bool>(base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
int32_t basic_value<CharT,Traits,Alloc>::to_int(size_t base) const
{
	return get<int32_t>(base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
uint32_t basic_value<CharT,Traits,Alloc>::to_uint(size_t base) const
{
	return get<uint32_t>(base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
int64_t basic_value<CharT,Traits,Alloc>::to_long(size_t base) const
{
	return get<int64_t>(base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
uint64_t basic_value<CharT,Traits,Alloc>::to_ulong(size_t base) const
{
	return get<uint64_t>(base);
}

template <concepts::character CharT, typename Traits, typename Alloc>
float basic_value<CharT,Traits,Alloc>::to_float() const
{
	return get<float>();
}

template <concepts::character CharT, typename Traits, typename Alloc>
double basic_value<CharT,Traits,Alloc>::to_double() const
{
	return get<double>();
}

template <concepts::character CharT, typename Traits, typename Alloc>
long double basic_value<CharT,Traits,Alloc>::to_ldouble() const
{
	return get<long double>();
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::to_bool_or(size_t base, bool def_value) const noexcept
{
	return get_or<bool>(base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
int32_t basic_value<CharT,Traits,Alloc>::to_int_or(size_t base, int32_t def_value) const noexcept
{
	return get_or<int32_t>(base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
uint32_t basic_value<CharT,Traits,Alloc>::to_uint_or(size_t base, uint32_t def_value) const noexcept
{
	return get_or<uint32_t>(base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
int64_t basic_value<CharT,Traits,Alloc>::to_long_or(size_t base, int64_t def_value) const noexcept
{
	return get_or<int64_t>(base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
uint64_t basic_value<CharT,Traits,Alloc>::to_ulong_or(size_t base, uint64_t def_value) const noexcept
{
	return get_or<uint64_t>(base, def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
float basic_value<CharT,Traits,Alloc>::to_float_or(float def_value) const noexcept
{
	return get_or<float>(def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
double basic_value<CharT,Traits,Alloc>::to_double_or(double def_value) const noexcept
{
	return get_or<double>(def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
long double basic_value<CharT,Traits,Alloc>::to_ldouble_or(long double def_value) const noexcept
{
	return get_or<long double>(def_value);
}

template <concepts::character CharT, typename Traits, typename Alloc>
template <typename Arg0, typename...Args>
void basic_value<CharT,Traits,Alloc>::set(format_string<Arg0, Args...> fmt, Arg0 &&arg0, Args&&...args)
{
	m_str = std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...);
}

template <concepts::character CharT, typename Traits, typename Alloc>
void basic_value<CharT,Traits,Alloc>::set(concepts::value_arg_p<char_t> auto &&arg)
{
	using Arg = decltype(arg);
	using arg_t = std::remove_cvref_t<Arg>;

	if constexpr( is_any_char_v<arg_t> or is_any_string_v<arg_t> )
		m_str = strtls::to_string(std::forward<Arg>(arg));
	else
		m_str = std::format(l_str(char_t,"{}"), std::forward<Arg>(arg));
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::is_alpha() const noexcept
{
	return strtls::is_alpha(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::is_digit() const noexcept
{
	return strtls::is_digit(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::is_rlnum() const noexcept
{
	return strtls::is_rlnum(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::is_alnum() const noexcept
{
	return strtls::is_alnum(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::is_ascii() const noexcept
{
	return strtls::is_ascii(m_str);
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::operator*() & noexcept
{
	return get();
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&
basic_value<CharT,Traits,Alloc>::operator*() const & noexcept
{
	return get();
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::operator*() && noexcept
{
	return std::move(get());
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t&&
basic_value<CharT,Traits,Alloc>::operator*() const && noexcept
{
	return std::move(get());
}

template <concepts::character CharT, typename Traits, typename Alloc>
typename basic_value<CharT,Traits,Alloc>::string_t*
basic_value<CharT,Traits,Alloc>::operator->() noexcept
{
	return &get();
}

template <concepts::character CharT, typename Traits, typename Alloc>
const typename basic_value<CharT,Traits,Alloc>::string_t*
basic_value<CharT,Traits,Alloc>::operator->() const noexcept
{
	return &get();
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::operator==(const str_view_t &str) const
{
	return m_str == str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
bool basic_value<CharT,Traits,Alloc>::operator==(const string_t &str) const
{
	return m_str == str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
auto basic_value<CharT,Traits,Alloc>::operator<=>(const basic_value &other) const
{
	return m_str <=> other.to_string();
}

template <concepts::character CharT, typename Traits, typename Alloc>
auto basic_value<CharT,Traits,Alloc>::operator<=>(const str_view_t &str) const
{
	return m_str <=> str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
auto basic_value<CharT,Traits,Alloc>::operator<=>(const string_t &str) const
{
	return m_str <=> str;
}

template <concepts::character CharT, typename Traits, typename Alloc>
basic_value<CharT,Traits,Alloc> &basic_value<CharT,Traits,Alloc>::operator=
(concepts::value_arg_p<char_t> auto &&arg)
{
	set(std::forward<decltype(arg)>(arg));
	return *this;
}

} //namespace libgs

namespace std
{

template <libgs::concepts::character CharT, typename...Args>
struct hash<libgs::basic_value<CharT,Args...>>
{
	size_t operator()(const libgs::basic_value<CharT,Args...> &v) const noexcept {
		return hash<std::basic_string<CharT,Args...>>()(v);
	}
};

template <libgs::concepts::character CharT>
struct formatter<libgs::basic_value<CharT>, CharT>
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
