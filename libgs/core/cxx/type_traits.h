// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_TYPE_TRAITS_H
#define LIBGS_CORE_CXX_TYPE_TRAITS_H

#include <libgs/core/cxx/configs.h>
#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/asio.h>

namespace libgs
{

using size_t  = std::size_t;

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

#if LIBGS_USING_BOOST_ASIO
using error_code = boost::system::error_code;
using error_category_t = boost::system::error_category;
#else //LIBGS_USING_BOOST_ASIO
using error_code = std::error_code;
using error_category_t = std::error_category;
#endif //LIBGS_USING_BOOST_ASIO

[[nodiscard]] inline error_code make_system_error_code(std::errc value) noexcept {
	return { std::make_error_code(value) };
}
namespace errc = asio::error;

template <size_t>
struct byte_type {};

template <> struct byte_type<1> { using unsigned_t = uint8_t ; using signed_t = int8_t ; };
template <> struct byte_type<2> { using unsigned_t = uint16_t; using signed_t = int16_t; };
template <> struct byte_type<4> { using unsigned_t = uint32_t; using signed_t = int32_t; };
template <> struct byte_type<8> { using unsigned_t = uint64_t; using signed_t = int64_t; };

template <size_t N> using byte_unsigned_t = byte_type<N>::unsigned_t;
template <size_t N> using byte_signed_t   = byte_type<N>::signed_t  ;

template <typename T>
struct sizeof_type : byte_type<sizeof(T)> {
	static constexpr size_t bytes = sizeof(T);
};

using uintptr_t = sizeof_type<void*>::unsigned_t;
using intptr_t  = sizeof_type<void*>::signed_t;

} //namespace libgs


#endif //LIBGS_CORE_CXX_TYPE_TRAITS_H
