
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

#ifndef LIBGS_CORE_CXX_TYPE_TRAITS_H
#define LIBGS_CORE_CXX_TYPE_TRAITS_H

#include <libgs/core/cxx/asio_concepts.h>
#include <libgs/core/cxx/coro_concepts.h>
#include <chrono>
#include <format>

namespace libgs
{

using size_t = std::size_t;

template<typename Rep, typename Period>
using duration = std::chrono::duration<Rep, Period>;

template<typename Clock, typename Duration>
using time_point = std::chrono::time_point<Clock, Duration>;

template <concepts::char_type CharT>
struct default_format {};

template <>
struct default_format<char> {
	static constexpr const char *value = "{}";
};

template <>
struct default_format<wchar_t> {
	static constexpr const wchar_t *value = L"{}";
};

template <concepts::char_type CharT, typename...Args>
using format_string = std::basic_format_string<CharT, std::type_identity_t<Args>...>;

template <concepts::char_type CharT>
static constexpr const CharT *default_format_v = default_format<CharT>::value;

using mutable_buffer = asio::ASIO_MUTABLE_BUFFER;

class const_buffer : public asio::ASIO_CONST_BUFFER
{
public:
	using asio::ASIO_CONST_BUFFER::ASIO_CONST_BUFFER;
	const_buffer &operator=(const const_buffer&) = default;

	const_buffer(const asio::ASIO_CONST_BUFFER &buf) :
		asio::ASIO_CONST_BUFFER(buf.data(), buf.size()) {}

	const_buffer(const mutable_buffer &buf) :
		asio::ASIO_CONST_BUFFER(buf.data(), buf.size()) {}

	const_buffer(const char *buf) :
		asio::ASIO_CONST_BUFFER(buf, strlen(buf)) {}

	const_buffer(const std::string &buf) :
		asio::ASIO_CONST_BUFFER(buf.c_str(), buf.size()) {}

	const_buffer(std::string_view &buf) :
		asio::ASIO_CONST_BUFFER(buf.data(), buf.size()) {}

	const_buffer &operator=(const mutable_buffer &buf)
	{
		operator=(const_buffer(buf.data(), buf.size()));
		return *this;
	}
};

template <typename...Args>
inline auto buffer(Args&&...args) {
	return asio::buffer(std::forward<Args>(args)...);
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_TYPE_TRAITS_H
