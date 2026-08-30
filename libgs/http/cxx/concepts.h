
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_CXX_CONCEPTS_H
#define LIBGS_HTTP_CXX_CONCEPTS_H

#include <libgs/core/global.h>

#if LIBGS_OPENSSL_SUPPORT
#include <asio/ssl.hpp>
#endif //LIBGS_OPENSSL_SUPPORT

namespace libgs::http
{

template <typename>
struct is_stream : std::false_type {};

template <concepts::exec Exec>
struct is_stream<asio::basic_stream_socket<asio::ip::tcp,Exec>> : std::true_type {};

#if LIBGS_OPENSSL_SUPPORT
template <concepts::exec Exec>
struct is_stream<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> : std::true_type {};
#endif //LIBGS_OPENSSL_SUPPORT

template <typename Stream>
constexpr bool is_stream_v = is_stream<Stream>::value;

template <typename Stream>
struct is_any_exec_stream
{
	static constexpr bool value = is_stream_v<Stream> and
		std::is_same_v<typename Stream::executor_type, asio::any_io_executor>;
};

template <typename Stream>
constexpr bool is_any_exec_stream_v = is_any_exec_stream<Stream>::value;

template <typename>
struct is_array_buffer : std::false_type {};

template <typename T, size_t N>
struct is_array_buffer<std::array<T,N>> : std::true_type {};

template <typename T>
constexpr bool is_array_buffer_v = is_array_buffer<T>::value;

template <typename>
struct is_vector_buffer : std::false_type {};

template <typename T>
struct is_vector_buffer<std::vector<T>> : std::true_type {};

template <typename T>
constexpr bool is_vector_buffer_v = is_vector_buffer<T>::value;

template <typename>
struct is_string_buffer : std::false_type {};

template <concepts::character CharT, typename Traits, typename Alloc>
struct is_string_buffer<std::basic_string<CharT,Traits,Alloc>> : std::true_type {};

template <typename T>
constexpr bool is_string_buffer_v = is_string_buffer<T>::value;

template <typename T>
struct is_buffer : std::disjunction <
	is_array_buffer<T>, is_vector_buffer<T>, is_string_buffer<T>
> {};

template <typename T>
constexpr bool is_buffer_v = is_buffer<T>::value;

namespace concepts
{

template <typename Stream>
concept stream = is_stream_v<Stream>;

template <typename Stream>
concept stream_p = is_stream_v<std::remove_cvref_t<Stream>>;

template <typename Stream>
concept any_exec_stream = is_any_exec_stream_v<Stream>;

template <typename Stream>
concept any_exec_stream_p = is_any_exec_stream_v<std::remove_cvref_t<Stream>>;

template <typename T>
concept array_buffer = is_array_buffer_v<T>;

template <typename T>
concept vector_buffer = is_vector_buffer_v<T>;

template <typename T>
concept string_buffer = is_string_buffer_v<T>;

template <typename T>
concept buffer = is_buffer_v<T>;

template <typename Token, typename...Args>
concept dis_detach_opt_token =
	libgs::concepts::tf_opt_token<Token,Args...> and
	not is_detached_v<std::remove_cvref_t<Token>>;

template <typename Func, typename Token>
concept progress_handler =
	libgs::concepts::callable<Func,size_t,size_t> and
	[]() consteval -> bool
	{
		using token_t = token_unbound_t<Token>;
		using return_t = decltype(std::declval<Func>()(0,0));

		if constexpr( (is_use_awaitable_v<token_t> or is_deferred_v<token_t>) and
			is_awaitable_v<return_t> )
		{
			using co_return_t = return_t::value_type;
			return std::is_same_v<co_return_t, bool> or
				   std::is_same_v<co_return_t, void>;
		}
		else
		{
			return std::is_same_v<return_t, bool> or
				   std::is_same_v<return_t, void>;
		}
		return false;
	}();

template <typename Func, typename Token>
concept progress_callback =
	progress_handler<Func,Token> and
	libgs::concepts::tf_opt_token<Token,error_code,size_t>;

} //namespace concepts

namespace core_concepts = libgs::concepts;

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_CONCEPTS_H
