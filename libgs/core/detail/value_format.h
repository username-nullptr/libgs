
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

#ifndef LIBGS_CORE_DETAIL_VALUE_SERIALIZER_H
#define LIBGS_CORE_DETAIL_VALUE_SERIALIZER_H

#include <libgs/core/global.h>

namespace libgs
{

template <concepts::character CharT,
		  typename Traits = std::char_traits<CharT>,
		  typename Alloc = std::allocator<CharT>>
class basic_value;

template <typename, concepts::character>
struct is_value : std::false_type {};

template <concepts::character CharT, typename...Args>
struct is_value<basic_value<CharT,Args...>,CharT> : std::true_type {};

template <typename T, concepts::character CharT>
constexpr bool is_value_v = is_value<T,CharT>::value;

template <typename T>
struct is_any_value : std::disjunction <
	is_value<T,char>, is_value<T,wchar_t>,
	is_value<T,char8_t>, is_value<T,char16_t>, is_value<T,char32_t>
> {};

template <typename T>
constexpr bool is_any_value_v = is_any_value<T>::value;

namespace concepts
{

template <typename T, typename CharT>
concept value = is_value_v<T,CharT>;

template <typename T, typename CharT>
concept value_p = is_value_v<std::remove_cvref_t<T>,CharT>;

template <typename T>
concept any_value = is_any_value_v<T>;

template <typename T>
concept any_value_p = is_any_value_v<std::remove_cvref_t<T>>;

} //namespace concepts

template <typename T, concepts::character CharT = char>
class LIBGS_CORE_TAPI value_default_serializer
{
public:
	constexpr std::basic_string<CharT> set(const T &data) {
		return std::format(l_str(CharT,"{}"), data);
	}
};

template <typename T, concepts::character CharT = char>
class LIBGS_CORE_TAPI value_serializer : public value_default_serializer<T,CharT> {};

template <concepts::integral T, concepts::character CharT>
class LIBGS_CORE_TAPI value_serializer<T,CharT> : public value_default_serializer<T,CharT>
{
public:
	constexpr T get(const basic_value<CharT> &value, size_t base = 10) {
		return strtls::to_arith<T>(*value, base);
	}
	constexpr T get_or(const basic_value<CharT> &value, T def_data, size_t base = 10) {
		return strtls::to_arith_or<T>(*value, def_data, base);
	}
};

template <concepts::floating T, concepts::character CharT>
class LIBGS_CORE_TAPI value_serializer<T,CharT> : public value_default_serializer<T,CharT>
{
public:
	constexpr T get(const basic_value<CharT> &value) {
		return strtls::to_arith<T>(*value);
	}
	constexpr T get_or(const basic_value<CharT> &value, T def_data) {
		return strtls::to_arith_or<T>(*value, def_data);
	}
};

template <concepts::enumerate T, concepts::character CharT>
class LIBGS_CORE_TAPI value_serializer<T,CharT> : public value_default_serializer<T,CharT>
{
public:
	constexpr T get(const basic_value<CharT> &value, size_t base) {
		return static_cast<T>(value_serializer<int,CharT>().get(value, base));
	}
	constexpr T get_or(const basic_value<CharT> &value, T def_data) {
		return static_cast<T>(value_serializer<int,CharT>().get_or(value, def_data));
	}
};

template <typename T, concepts::character CharT> requires concepts::string<T,CharT>
class LIBGS_CORE_TAPI value_serializer<T,CharT>
{
public:
	constexpr decltype(auto) set(concepts::string_p<CharT> auto &&data) {
		return return_reference(std::forward<decltype(data)>(data));
	}
	constexpr decltype(auto) get(concepts::value_p<CharT> auto &&value)
	{
		using Value = decltype(value);
		if constexpr( is_std_string_v<T,CharT> or std::is_rvalue_reference_v<Value> )
			return return_reference(*std::forward<Value>(value));
		else
			return std::basic_string_view<CharT>(*value);
	}
};

template <concepts::character CharT>
class LIBGS_CORE_TAPI value_serializer<basic_value<CharT>,CharT>
{
public:
	constexpr decltype(auto) get(concepts::value_p<CharT> auto &&value) {
		return return_reference(std::forward<decltype(value)>(value));
	}
};

namespace concepts
{

template <typename T, typename CharT>
concept value_set = requires (
	value_serializer<std::remove_cvref_t<T>,CharT> serializer,
	std::basic_string<CharT> &str, const T &data) {
	str = serializer.set(data);
};

template <typename T>
concept any_value_set =
	value_set<T,char> or value_set<T,wchar_t> or
	value_set<T,char8_t> or value_set<T,char16_t> or value_set<T,char32_t>;

template <typename CharT, typename T, typename...Args>
concept value_get = requires (
	value_serializer<std::remove_cvref_t<T>,CharT> serializer,
	const basic_value<CharT> &value, Args&&...args) {
	serializer.get(value, std::forward<Args>(args)...);
};

template <typename T, typename...Args>
concept any_value_get =
	value_get<char,T,Args...> or value_get<wchar_t,T,Args...> or
	value_get<char8_t,T,Args...> or value_get<char16_t,T,Args...> or value_get<char32_t,T,Args...>;

template <typename CharT, typename T, typename...Args>
concept value_get_or = requires (
	value_serializer<std::remove_cvref_t<T>,CharT> serializer,
	const basic_value<CharT> &value, T &&def_data, Args&&...args) {
	serializer.get_or(value, std::forward<T>(def_data), std::forward<Args>(args)...);
};

template <typename T, typename...Args>
concept any_value_get_or =
	value_get_or<char,T,Args...> or value_get_or<wchar_t,T,Args...> or
	value_get_or<char8_t,T,Args...> or value_get_or<char16_t,T,Args...> or value_get_or<char32_t,T,Args...>;

}} //namespace libgs::concepts


#endif //LIBGS_CORE_DETAIL_VALUE_SERIALIZER_H