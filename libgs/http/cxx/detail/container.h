
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

#ifndef LIBGS_HTTP_CXX_DETAIL_CONTAINER_H
#define LIBGS_HTTP_CXX_DETAIL_CONTAINER_H

namespace libgs::http
{

inline bool less_case_insensitive::operator()(const key_t &v1, const key_t &v2) const
{
	return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end(), [](char c1, char c2){
		return std::tolower(c1) < std::tolower(c2);
	});
}

template <typename T>
decltype(auto) value_map_get
(const value_map &map, const core_concepts::text_p<char> auto &key, const char *msg)
	requires core_concepts::value_get<T,char>
{
	auto key_str = strtls::to_string(key);
	auto it = map.find(key_str);

	if( it == map.end() )
	{
		throw runtime_error (
			"{}: key '{}' not exists.", msg, key_str
		);
	}
	return it->second.template get<T>();
}

template <typename T>
decltype(auto) value_map_get_or
(const value_map &map, const core_concepts::text_p<char> auto &key, T &&def_value)
	requires core_concepts::value_get<T,char>
{
	auto it = map.find(strtls::to_string(key));
	using def_t = std::remove_cvref_t<T>;

	if constexpr( is_string_v<def_t, char> )
	{
		return it == map.end() ?
			strtls::to_string(std::forward<T>(def_value)) : *it->second;
	}
	else
	{
		return it == map.end() ? std::forward<T>(def_value) :
			it->second.template get<def_t>();
	}
}

template <typename T>
decltype(auto) value_set_get(const value_set &set, const value &node, const char *msg)
	requires core_concepts::value_get<T,char>
{
	auto it = set.find(node);
	if( it == set.end() )
	{
		throw runtime_error (
			"{}: node '{}' not exists.", msg, node
		);
	}
	return it->template get<T>();
}

template <typename T>
decltype(auto) value_set_get_or(const value_set &set, value &node, T &&def_value)
	requires core_concepts::value_get<T,char>
{
	auto it = set.find(node);
	using def_t = std::remove_cvref_t<T>;

	if constexpr( is_string_v<def_t, char> )
	{
		return it == set.end() ?
			strtls::to_string(std::forward<T>(def_value)) : *it;
	}
	else
	{
		return it == set.end() ? std::forward<T>(def_value) :
			it->template get<def_t>();
	}
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_CONTAINER_H
