
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

template <core_concepts::char_type CharT>
using basic_map = std::map <
	std::basic_string<CharT>, basic_value<CharT>,
	basic_less_case_insensitive<CharT>
>;

using map  = basic_map<char>;
using wmap = basic_map<wchar_t>;

template <core_concepts::char_type CharT>
using basic_map_init_list = std::initializer_list<std::tuple <
	std::basic_string_view<CharT>, basic_value<CharT>
>>;

using map_init_list  = basic_map_init_list<char>;
using wmap_init_list = basic_map_init_list<wchar_t>;

template <core_concepts::char_type CharT>
using basic_map_key_init_list = std::initializer_list <
	std::basic_string_view<CharT>
>;

using map_key_init_list  = basic_map_key_init_list<char>;
using wmap_key_init_list = basic_map_key_init_list<wchar_t>;

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
using basic_set_init_list = std::initializer_list <
	basic_value<CharT>
>;

using set_init_list  = basic_set_init_list<char>;
using wset_init_list = basic_set_init_list<wchar_t>;

namespace concepts
{

template <typename CharT, typename...Args>
concept set_map_params = core_concepts::container_params <
	std::tuple<std::basic_string_view<CharT>, basic_value<CharT>>, Args...
>;

template <typename CharT, typename...Args>
concept unset_map_params = core_concepts::container_params <
	std::basic_string_view<CharT>, Args...
>;

template <typename CharT, typename...Args>
concept set_set_params = core_concepts::container_params <
	basic_value<CharT>, Args...
>;

template <typename CharT, typename...Args>
concept unset_set_params = core_concepts::container_params <
	basic_value<CharT>, Args...
>;

} //namespace concepts

template <core_concepts::char_type CharT, typename...Args>
void set_map(basic_map<CharT> &map, Args&&...args) noexcept
	requires concepts::set_map_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void set_map(basic_map<CharT> &map, basic_map_init_list<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename...Args>
void unset_map(basic_map<CharT> &map, Args&&...args) noexcept
	requires concepts::unset_map_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void unset_map(basic_map<CharT> &map, basic_map_key_init_list<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename...Args>
void set_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::set_set_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void set_set(basic_set<CharT> &set, basic_set_init_list<CharT> list) noexcept;

template <core_concepts::char_type CharT, typename...Args>
void unset_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::unset_set_params<CharT,Args...>;

template <core_concepts::char_type CharT>
void unset_set(basic_set<CharT> &set, basic_set_init_list<CharT> list) noexcept;

} //namespace libgs::http
#include <libgs/http/cxx/detail/container.h>


#endif //LIBGS_HTTP_CXX_CONTAINER_H
