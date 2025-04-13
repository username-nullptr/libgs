
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

struct LIBGS_HTTP_TAPI less_case_insensitive {
	bool operator()(const std::string &v1, const std::string &v2) const;
};

template <typename Value>
using map = std::map <
	std::string, Value, less_case_insensitive
>;
using attr_map = map<value>;

template <typename Value>
using pair_init = std::initializer_list <
	std::tuple<std::string_view, Value>
>;
using key_attr_init = pair_init<value>;

using key_init = std::initializer_list <
	std::string_view
>;

using set = std::set <
	value, less_case_insensitive
>;
using value_set = set;
using attr_init = std::initializer_list<value>;

namespace concepts
{

template <typename Value, typename...Args>
concept set_pair_params = core_concepts::container_params <
	std::tuple<std::string, Value>, Args...
>;

template <typename...Args>
concept set_key_attr_params = set_pair_params<value,Args...>;

template <typename...Args>
concept unset_pair_params = core_concepts::container_params<std::string,Args...>;

template <typename...Args>
concept set_attr_params = core_concepts::container_params<value,Args...>;

template <typename...Args>
concept unset_attr_params = core_concepts::container_params<value,Args...>;

} //namespace concepts

template <typename Value, typename...Args>
LIBGS_HTTP_TAPI void set_map(map<Value> &map, Args&&...args) noexcept
	requires concepts::set_pair_params<Value,Args...>;

template <typename Value>
LIBGS_HTTP_TAPI void set_map(map<Value> &map, pair_init<Value> list) noexcept;

template <typename Value, typename...Args>
LIBGS_HTTP_TAPI void unset_map(map<Value> &map, Args&&...args) noexcept
	requires concepts::unset_pair_params<Args...>;

template <typename Value>
LIBGS_HTTP_TAPI void unset_map(map<Value> &map, key_init list) noexcept;

template <typename...Args>
LIBGS_HTTP_TAPI void set_set(set &set, Args&&...args) noexcept
	requires concepts::set_attr_params<Args...>;

LIBGS_HTTP_VAPI void set_set(set &set, attr_init list) noexcept;

template <typename...Args>
LIBGS_HTTP_TAPI void unset_set(set &set, Args&&...args) noexcept
	requires concepts::unset_attr_params<Args...>;

LIBGS_HTTP_VAPI void unset_set(set &set, attr_init list) noexcept;

template <typename Value>
[[nodiscard]] LIBGS_HTTP_TAPI Value &get_map_value (
	const map<Value> &map, core_concepts::string_type auto &&key
);

template <typename Value, typename Default>
[[nodiscard]] LIBGS_HTTP_TAPI decltype(auto) get_map_value_or(const map<Value> &map,
	core_concepts::string_type auto &&key, Default &&def_value
) requires std::is_same_v<Value,std::remove_cvref_t<Default>>;

template <core_concepts::text_arg_p T = value>
[[nodiscard]] LIBGS_HTTP_TAPI decltype(auto) get_attr_map_value(const attr_map &map,
	core_concepts::string_type auto &&key
);

template <core_concepts::text_arg_p T = value>
[[nodiscard]] LIBGS_HTTP_TAPI decltype(auto) get_attr_map_value_or(const attr_map &map,
	core_concepts::string_type auto &&key, T &&def_value
);

template <typename Value>
class LIBGS_HTTP_TAPI map_helper
{
public:
	using value_t = Value;
	map<value_t> map;

	template <typename...Args>
	map_helper(Args&&...args) noexcept requires
		concepts::set_key_attr_params<Args...>;

	map_helper(key_attr_init headers) noexcept;
	map_helper() = default;
};

using attr_map_helper = map_helper<value>;

} //namespace libgs::http
#include <libgs/http/cxx/detail/container.h>


#endif //LIBGS_HTTP_CXX_CONTAINER_H
