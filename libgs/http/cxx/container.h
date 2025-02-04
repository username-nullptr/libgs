
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

#ifndef LIBGS_HTTP_CXX_CONTAINER_H
#define LIBGS_HTTP_CXX_CONTAINER_H

#include <libgs/http/cxx/attributes.h>
#include <libgs/http/cxx/concepts.h>
#include <libgs/core/value.h>
#include <ranges>
#include <map>
#include <set>

namespace libgs::http
{

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI basic_less_case_insensitive
{
	using string_t = std::basic_string<CharT>;
	bool operator()(const string_t &v1, const string_t &v2) const;
};

using less_case_insensitive = basic_less_case_insensitive<char>;
using wless_case_insensitive = basic_less_case_insensitive<wchar_t>;

template <core_concepts::char_type CharT, typename Value>
using basic_map = std::map <
	std::basic_string<CharT>, Value,
	basic_less_case_insensitive<CharT>
>;

template <typename V>
using map = basic_map<char,V>;

template <typename V>
using wmap = basic_map<wchar_t,V>;

template <core_concepts::char_type CharT>
using basic_attr_map = basic_map<CharT,basic_value<CharT>>;

using attr_map  = basic_attr_map<char>;
using wattr_map = basic_attr_map<wchar_t>;

template <core_concepts::char_type CharT, typename Value>
using basic_pair_init = std::initializer_list<std::tuple <
	std::basic_string_view<CharT>, Value
>>;

template <typename V>
using pair_init = basic_pair_init<char,V>;

template <typename V>
using wpair_init = basic_pair_init<wchar_t,V>;

template <core_concepts::char_type CharT>
using basic_key_attr_init = basic_pair_init<CharT,basic_value<CharT>>;

using key_attr_init  = basic_key_attr_init<char>;
using wkey_attr_init = basic_key_attr_init<wchar_t>;

template <core_concepts::char_type CharT>
using basic_key_init = std::initializer_list <
	std::basic_string_view<CharT>
>;

using key_init_list  = basic_key_init<char>;
using wkey_init_list = basic_key_init<wchar_t>;

template <core_concepts::char_type CharT>
using basic_set = std::set <
	basic_value<CharT>,
	basic_less_case_insensitive<CharT>
>;

using set  = basic_set<char>;
using wset = basic_set<wchar_t>;

template <core_concepts::char_type CharT>
using basic_value_set = basic_set<CharT>;

using value_set  = basic_value_set<char>;
using wvalue_set = basic_value_set<wchar_t>;

template <core_concepts::char_type CharT>
using basic_attr_init = std::initializer_list <
	basic_value<CharT>
>;

using attr_init  = basic_attr_init<char>;
using wattr_init = basic_attr_init<wchar_t>;

namespace concepts
{

template <typename CharT, typename Value, typename...Args>
concept set_pair_params = core_concepts::container_params <
	std::tuple<std::basic_string<CharT>, Value>, Args...
>;

template <typename CharT, typename...Args>
concept set_key_attr_params = set_pair_params <
	CharT, basic_value<CharT>, Args...
>;

template <typename CharT, typename...Args>
concept unset_pair_params = core_concepts::container_params <
	std::basic_string<CharT>, Args...
>;

template <typename CharT, typename...Args>
concept set_attr_params = core_concepts::container_params <
	basic_value<CharT>, Args...
>;

template <typename CharT, typename...Args>
concept unset_attr_params = core_concepts::container_params <
	basic_value<CharT>, Args...
>;

} //namespace concepts

template <core_concepts::char_type CharT, typename Value, typename...Args>
void set_map(basic_map<CharT,Value> &map, Args&&...args) noexcept
	requires concepts::set_pair_params<CharT,Value,Args...>;

template <core_concepts::char_type CharT, typename Value>
void set_map(basic_map<CharT,Value> &map, basic_pair_init<CharT,Value> list) noexcept;

template <core_concepts::char_type CharT, typename Value, typename...Args>
void unset_map(basic_map<CharT,Value> &map, Args&&...args) noexcept
	requires concepts::unset_pair_params<CharT,Args...>;

template <core_concepts::char_type CharT, typename Value>
void unset_map(basic_map<CharT,Value> &map, basic_key_init<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename...Args>
void set_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::set_attr_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void set_set(basic_set<CharT> &set, basic_attr_init<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename...Args>
void unset_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::unset_attr_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void unset_set(basic_set<CharT> &set, basic_attr_init<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename Value>
[[nodiscard]] Value &get_map_value(const basic_map<CharT,Value> &map,
	core_concepts::basic_string_type<CharT> auto &&key
);

template <core_concepts::char_type CharT, typename Value, typename Default>
[[nodiscard]] decltype(auto) get_map_value_or(const basic_map<CharT,Value> &map,
	core_concepts::basic_string_type<CharT> auto &&key, Default &&def_value
) requires std::is_same_v<Value,std::remove_cvref_t<Default>>;

template <core_concepts::char_type CharT,
	core_concepts::basic_text_arg<CharT> T = basic_value<CharT>>
[[nodiscard]] decltype(auto) get_attr_map_value(const basic_attr_map<CharT> &map,
	core_concepts::basic_string_type<CharT> auto &&key
);

template <core_concepts::char_type CharT,
	core_concepts::basic_text_arg<CharT> T = basic_value<CharT>>
[[nodiscard]] decltype(auto) get_attr_map_value_or(const basic_attr_map<CharT> &map,
	core_concepts::basic_string_type<CharT> auto &&key, T &&def_value
);

} //namespace libgs::http
#include <libgs/http/cxx/detail/container.h>


#endif //LIBGS_HTTP_CXX_CONTAINER_H
