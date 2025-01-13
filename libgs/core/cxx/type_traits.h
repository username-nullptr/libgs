
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
#include <libgs/core/cxx/attributes.h>
#include <chrono>
#include <format>

namespace libgs
{

using size_t = std::size_t;

template<typename Rep, typename Period>
using duration = std::chrono::duration<Rep, Period>;

using nanoseconds  = std::chrono::nanoseconds ;
using microseconds = std::chrono::microseconds;
using milliseconds = std::chrono::milliseconds;

using seconds = std::chrono::seconds;
using minutes = std::chrono::minutes;
using hours   = std::chrono::hours  ;

using days   = std::chrono::days  ;
using weeks  = std::chrono::weeks ;
using months = std::chrono::months;
using years  = std::chrono::years ;

template<typename Clock, typename Duration>
using time_point = std::chrono::time_point<Clock, Duration>;

using error_code = asio::error_code;
namespace errc = asio::error;

template <concepts::char_type>
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

class LIBGS_CORE_VAPI const_buffer : public asio::ASIO_CONST_BUFFER
{
public:
	using asio::ASIO_CONST_BUFFER::ASIO_CONST_BUFFER;
	const_buffer &operator=(const const_buffer&) = default;
	const_buffer(const asio::ASIO_CONST_BUFFER &buf);
	const_buffer(const mutable_buffer &buf);
	const_buffer(const char *buf);
	const_buffer(const std::string &buf);
	const_buffer(std::string_view buf);
	const_buffer &operator=(const mutable_buffer &buf);
};

template <typename...Args>
[[nodiscard]] LIBGS_CORE_TAPI auto buffer(Args&&...args);

template <size_t N>
struct byte_type {};

template <> struct byte_type<1> { using unsigned_t = uint8_t ; using signed_t = int8_t ; };
template <> struct byte_type<2> { using unsigned_t = uint16_t; using signed_t = int16_t; };
template <> struct byte_type<4> { using unsigned_t = uint32_t; using signed_t = int32_t; };
template <> struct byte_type<8> { using unsigned_t = uint64_t; using signed_t = int64_t; };

template <size_t N> using byte_unsigned_t = typename byte_type<N>::unsigned_t;
template <size_t N> using byte_signed_t   = typename byte_type<N>::signed_t  ;

template <typename T>
struct sizeof_type : byte_type<sizeof(T)> {
	static constexpr size_t bytes = sizeof(T);
};

using uintptr_t = sizeof_type<void*>::unsigned_t;
using intptr_t  = sizeof_type<void*>::signed_t;

} //namespace libgs
#include <libgs/core/cxx/detail/type_traits.h>


#endif //LIBGS_CORE_CXX_TYPE_TRAITS_H
