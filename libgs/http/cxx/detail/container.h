
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

template <core_concepts::char_type CharT>
bool basic_less_case_insensitive<CharT>::operator()(const string_t &v1, const string_t &v2) const
{
	return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end(), [](CharT c1, CharT c2){
		return tolower(c1) < tolower(c2);
	});
}

template <core_concepts::char_type CharT, typename Value, typename...Args>
void set_map(basic_map<CharT,Value> &map, Args&&...args) noexcept
	requires concepts::set_pair_params<CharT,Value,Args...>
{
	using string_view_t = std::basic_string_view<CharT>;
	using tuple_t = std::tuple<string_view_t,Value>;

	if constexpr( core_concepts::constructible<tuple_t,Args...> )
	{
		tuple_t tuple(std::forward<Args>(args)...);
		map[str_to_lower(std::get<0>(tuple))] = std::move(std::get<1>(tuple));
	}
	else if constexpr( sizeof...(Args) == 1 )
	{
		decltype(auto) container = std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
		for(auto &[key,value] : container)
			map[str_to_lower(key)] = std::move(value);
	}
	else
	{
		auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
		using iter_t = match_iterator_t<std::tuple_element_t<0,decltype(tuple)>>;

		iter_t it = std::get<0>(tuple);
		iter_t end = std::get<1>(tuple);
		for(; it!=end; ++it)
		{
			auto &[key,value] = *it;
			map[str_to_lower(key)] = std::move(value);
		}
	}
}

template <core_concepts::char_type CharT, typename Value>
void set_map(basic_map<CharT,Value> &map, basic_pair_init<CharT,Value> list) noexcept
{
	for(auto &[key,value] : list)
		map[str_to_lower(key)] = std::move(value);
}

template <core_concepts::char_type CharT, typename Value, typename...Args>
void unset_map(basic_map<CharT,Value> &map, Args&&...args) noexcept
	requires concepts::unset_pair_params<CharT,Args...>
{
	using string_t = std::basic_string<CharT>;
	if constexpr( core_concepts::constructible<string_t,Args...> )
	{
		if constexpr( sizeof...(Args) > 1 )
			map.erase(string_t(std::forward<Args>(args)...));
		else
		{
			using tuple_t = std::tuple<Args...>;
			using type = std::tuple_element_t<0,tuple_t>;

			if constexpr( std::is_same_v<type,string_t> )
				map.erase(std::forward<Args>(args)...);
			else
				map.erase(string_t(std::forward<Args>(args)...));
		}
	}
	else
	{
		auto erase = [&](auto &key)
		{
			using key_t = std::remove_cvref_t<decltype(key)>;
			if constexpr( std::is_same_v<key_t,string_t> )
				map.erase(key);
			else
				map.erase(string_t(key));
		};
		if constexpr( sizeof...(Args) == 1 )
		{
			decltype(auto) container = std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
			for(auto &key : container)
				erase(key);
		}
		else
		{
			auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
			using iter_t = match_iterator_t<std::tuple_element_t<0,decltype(tuple)>>;

			iter_t it = std::get<0>(tuple);
			iter_t end = std::get<1>(tuple);
			for(; it!=end; ++it)
				erase(*it);
		}
	}
}

template <core_concepts::char_type CharT, typename Value>
void unset_map(basic_map<CharT,Value> &map, basic_key_init<CharT> list) noexcept
{
	for(auto &key : list)
		map.erase(std::basic_string<CharT>(key));
}

template <core_concepts::char_type CharT, typename...Args>
void set_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::set_attr_params<CharT,Args...>
{
	using value_t = basic_value<CharT>;
	if constexpr( core_concepts::constructible<value_t,Args...> )
	{
		if constexpr( sizeof...(Args) > 1 )
			set.emplace(value_t(std::forward<Args>(args)...));
		else
		{
			using tuple_t = std::tuple<Args...>;
			using type = std::tuple_element_t<0,tuple_t>;

			if constexpr( std::is_same_v<type,value_t> )
				set.emplace(std::forward<Args>(args)...);
			else
				set.emplace(value_t(std::forward<Args>(args)...));
		}
	}
	else if constexpr( sizeof...(Args) == 1 )
	{
		decltype(auto) container = std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
		for(auto &attr : container)
			set.emplace(std::move(attr));
	}
	else
	{
		auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
		using iter_t = match_iterator_t<std::tuple_element_t<0,decltype(tuple)>>;

		iter_t it = std::get<0>(tuple);
		iter_t end = std::get<1>(tuple);
		for(; it!=end; ++it)
			set.emplace(std::move(*it));
	}
}

template <core_concepts::char_type CharT>
void set_set(basic_set<CharT> &set, basic_attr_init<CharT> list) noexcept
{
	for(auto &value : list)
		set.emplace(std::move(value));
}

template <core_concepts::char_type CharT, typename...Args>
void unset_set(basic_set<CharT> &set, Args&&...args) noexcept
	requires concepts::unset_attr_params<CharT,Args...>
{
	using value_t = basic_value<CharT>;
	if constexpr( core_concepts::constructible<value_t,Args...> )
	{
		if constexpr( sizeof...(Args) > 1 )
			set.erase(value_t(std::forward<Args>(args)...));
		else
		{
			using tuple_t = std::tuple<Args...>;
			using type = std::tuple_element_t<0,tuple_t>;

			if constexpr( std::is_same_v<type,value_t> )
				set.erase(std::forward<Args>(args)...);
			else
				set.erase(value_t(std::forward<Args>(args)...));
		}
	}
	else
	{
		auto erase = [&](auto &attr)
		{
			using attr_t = std::remove_cvref_t<decltype(attr)>;
			if constexpr( std::is_same_v<attr_t,value_t> )
				set.erase(attr);
			else
				set.erase(value_t(attr));
		};
		if constexpr( sizeof...(Args) == 1 )
		{
			decltype(auto) container = std::get<0>(std::forward_as_tuple(std::forward<Args>(args)...));
			for(auto &key : container)
				erase(key);
		}
		else
		{
			auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
			using iter_t = match_iterator_t<std::tuple_element_t<0,decltype(tuple)>>;

			iter_t it = std::get<0>(tuple);
			iter_t end = std::get<1>(tuple);
			for(; it!=end; ++it)
				erase(*it);
		}
	}
}

template <core_concepts::char_type CharT>
void unset_set(basic_set<CharT> &set, basic_attr_init<CharT> list) noexcept
{
	for(auto &value : list)
		set.erase(value);
}

template <core_concepts::char_type CharT, typename Value>
[[nodiscard]] Value &get_map_value
(const basic_map<CharT,Value> &map, core_concepts::basic_string_type<CharT> auto &&key)
{
	auto it = map.find(nosview(key));
	if( it == map.end() )
	{
		throw runtime_error("libgs::http::get_map_value: The key '{}' is not exists.",
			xxtombs(std::forward<decltype(key)>(key))
		);
	}
	return as_const(it->second);
}

template <core_concepts::char_type CharT, typename Value, typename Default>
[[nodiscard]] decltype(auto) get_map_value_or
(const basic_map<CharT,Value> &map, core_concepts::basic_string_type<CharT> auto &&key, Default &&def_value)
	requires std::is_same_v<Value,std::remove_cvref_t<Default>>
{
	auto it = map.find(nosview(key));
	return it == map.end() ? std::forward<Default>(def_value) : it->second;
}

template <core_concepts::char_type CharT, core_concepts::basic_text_arg<CharT> T>
decltype(auto) get_attr_map_value
(const basic_attr_map<CharT> &map, core_concepts::basic_string_type<CharT> auto &&key)
{
	decltype(auto) value = get_map_value(map, std::forward<decltype(key)>(key));
	using def_t = std::remove_cvref_t<T>;

	if constexpr( std::is_same_v<def_t, basic_value<CharT>> )
		return as_const(value);
	else
		return as_const(value.template get<def_t>());
}

template <core_concepts::char_type CharT, core_concepts::basic_text_arg<CharT> T>
decltype(auto) get_attr_map_value_or
(const basic_attr_map<CharT> &map, core_concepts::basic_string_type<CharT> auto &&key, T &&def_value)
{
	using value_t = basic_value<CharT>;
	using def_t = std::remove_cvref_t<T>;

	auto it = map.find(nosview(key));
	if constexpr( std::is_same_v<def_t, value_t> )
		return it == map.end() ? std::forward<T>(def_value) : it->second;
	else
	{
		return it == map.end() ?
			value_t(std::forward<T>(def_value)).template get<def_t>() :
			it->second.template get<def_t>();
	}
}

template <core_concepts::char_type CharT, typename Value>
template <typename...Args>
basic_map_helper<CharT,Value>::basic_map_helper(Args&&...args) noexcept requires
	concepts::set_key_attr_params<char_t,Args...>
{
	set_map(map, std::forward<Args>(args)...);
}

template <core_concepts::char_type CharT, typename Value>
basic_map_helper<CharT,Value>::basic_map_helper(pair_init_t headers) noexcept
{
	set_map(map, std::move(headers));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_CONTAINER_H
