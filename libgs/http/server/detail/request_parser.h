
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H
#define LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H

namespace libgs::http
{

template <typename T>
decltype(auto) request_parser::parameter(const core_concepts::text_p<char> auto &key)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get (
		parameters(), key, "libgs::http::request_parser::parameter"
	);
}

template <typename T>
decltype(auto) request_parser::parameter_or(const core_concepts::text_p<char> auto &key, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get_or (
		parameters(), key, std::forward<T>(def_value)
	);
}

template <typename T>
decltype(auto) request_parser::header(const core_concepts::text_p<char> auto &key)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get (
		headers(), key, "libgs::http::request_parser::header"
	);
}

template <typename T>
decltype(auto) request_parser::header_or(const core_concepts::text_p<char> auto &key, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get_or (
		headers(), key, std::forward<T>(def_value)
	);
}

template <typename T>
decltype(auto) request_parser::cookie(const core_concepts::text_p<char> auto &key)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get (
		cookies(), key, "libgs::http::request_parser::cookie"
	);
}

template <typename T>
decltype(auto) request_parser::cookie_or(const core_concepts::text_p<char> auto &key, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get_or (
		cookies(), key, std::forward<T>(def_value)
	);
}

template <typename T>
decltype(auto) request_parser::path_arg(const core_concepts::text_p<char> auto &key)
	const requires core_concepts::value_get<T,char>
{
	auto it = std::ranges::find(path_args(), key, [](const auto &pair) {
		return return_reference(pair.first);
	});
	if( it == path_args().end() )
	{
		throw runtime_error (
			"libgs::http::request_parser::path_arg: key '{}' not exists.", key
		);
	}
	return it->second.template get<T>();
}

template <typename T>
decltype(auto) request_parser::path_arg(size_t index)
	const requires core_concepts::value_get<T,char>
{
	if( index >= path_args().size() )
	{
		throw runtime_error (
			"libgs::http::request_parser::path_arg: index out of range."
		);
	}
	return path_args()[index].second.get<T>();
}

template <typename T>
decltype(auto) request_parser::path_arg_or(const core_concepts::text_p<char> auto &key, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	auto it = std::ranges::find(path_args(), key, [](const auto &pair) {
		return return_reference(pair.first);
	});
	using def_t = std::remove_cvref_t<T>;

	if constexpr( is_string_v<def_t, char> )
	{
		return it == path_args().end() ?
			strtls::to_string(std::forward<T>(def_value)) : *it->second;
	}
	else
	{
		return it == path_args().end() ? std::forward<T>(def_value) :
			it->second.template get<def_t>();
	}
}

template <typename T>
decltype(auto) request_parser::path_arg_or(size_t index, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	if( index < path_args().size() )
		return path_args()[index].second.get<T>();

	using def_t = std::remove_cvref_t<T>;
	if constexpr( is_string_v<def_t, char> )
		return strtls::to_string(std::forward<T>(def_value));
	else
		return std::forward<T>(def_value);
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_PARSER_H
