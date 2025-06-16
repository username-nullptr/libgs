
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

#ifndef LIBGS_HTTP_PROTOCOL_DETAIL_COOKIE_H
#define LIBGS_HTTP_PROTOCOL_DETAIL_COOKIE_H

namespace libgs::http::protocol
{

template <typename T>
decltype(auto) cookie::value() requires core_concepts::value_get<T,char>
{
	return value().get<T>();
}

template <typename T>
decltype(auto) cookie::attribute(const core_concepts::text_p<char> auto &key)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get (
		attributes(), key, "libgs::http::cookie::attributes"
	);
}

template <typename T>
decltype(auto) cookie::attribute_or
(const core_concepts::text_p<char> auto &key, T &&def_value)
	const requires core_concepts::value_get<T,char>
{
	return value_map_get_or (
		attributes(), key, std::forward<T>(def_value)
	);
}

cookie &cookie::set_attribute(core_concepts::text_p<char> auto &&key, value_t attr) noexcept
{
	attributes()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(attr);
	return *this;
}

cookie &cookie::unset_attribute(const core_concepts::text_p<char> auto &key) noexcept
{
	attributes().erase(strtls::to_string(key));
	return *this;
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_DETAIL_COOKIE_H
