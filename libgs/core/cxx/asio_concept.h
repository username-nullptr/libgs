
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_ASIO_CONCEPT_H
#define LIBGS_CORE_CXX_ASIO_CONCEPT_H

#include <libgs/core/cxx/function_traits.h>
#include <concepts>

#ifdef LIBGS_USING_BOOST_ASIO

# include <boost/asio.hpp>
# ifdef LIBGS_ENABLE_OPENSSL
#  include <boost/asio/ssl.hpp>
# endif //LIBGS_ENABLE_OPENSSL

namespace asio = boost::asio;

#else

# include <asio.hpp>
# ifdef LIBGS_ENABLE_OPENSSL
#  include <asio/ssl.hpp>
# endif //LIBGS_ENABLE_OPENSSL

#endif //LIBGS_USING_BOOST_ASIO

namespace libgs
{

template <typename Exec>
struct is_execution {
	static constexpr bool value =
		(asio::execution::is_executor<Exec>::value and
		 asio::can_require<Exec, asio::execution::blocking_t::never_t>::value) or
		 asio::is_executor<Exec>::value;
};

template <typename Exec>
constexpr bool is_execution_v = is_execution<Exec>::value;

template <typename ExecContext>
struct is_execution_context {
	static constexpr bool value = asio::is_convertible<ExecContext&, asio::execution_context&>::value;
};

template <typename ExecContext>
constexpr bool is_execution_context_v = is_execution_context<ExecContext>::value;

template <typename Exec>
struct is_schedulable {
	static constexpr bool value = is_execution_v<Exec> or is_execution_context_v<Exec>;
};

template <typename Exec>
constexpr bool is_schedulable_v = is_schedulable<Exec>::value;

namespace concepts
{

template <typename Exec, typename NativeExec>
concept match_execution =
	is_execution_v<Exec> and is_execution_v<NativeExec> and
	requires(Exec &exec) { NativeExec(exec); };

template <typename Exec, typename NativeExec>
concept match_execution_context =
	is_execution_context_v<Exec> and is_execution_v<NativeExec> and
	requires(Exec &exec) { NativeExec(exec.get_executor()); };

template <typename Exec, typename NativeExec>
concept match_execution_or_context =
	match_execution<Exec,NativeExec> or
	match_execution_context<Exec,NativeExec>;

} //namespace concepts

template <typename Exec, typename NativeExec>
struct is_match_execution {
	static constexpr bool value = concepts::match_execution<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_execution_v = is_match_execution<Exec,NativeExec>::value;

template <typename Exec, typename NativeExec>
struct is_match_execution_context {
	static constexpr bool value = concepts::match_execution_context<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_execution_context_v = is_match_execution_context<Exec,NativeExec>::value;

template <typename Exec, typename NativeExec>
struct is_match_execution_or_context {
	static constexpr bool value = concepts::match_execution_or_context<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_execution_or_context_v = is_match_execution_or_context<Exec,NativeExec>::value;

template <typename T>
struct is_awaitable : public std::false_type {};

template <typename T>
struct is_awaitable<asio::awaitable<T>> : public std::true_type {};

template <typename T>
constexpr bool is_awaitable_v = is_awaitable<T>::value;

namespace concepts
{

template <typename T>
concept awaitable_type = is_awaitable_v<T>;

} //namespace concepts

template <typename T>
struct awaitable_return_type {};

template <typename T>
struct awaitable_return_type<asio::awaitable<T>> {
	using type = T;
};

template <concepts::awaitable_type T>
using awaitable_return_type_t = typename awaitable_return_type<T>::type;

namespace concepts
{

template <typename Exec>
concept execution = is_execution_v<Exec>;

template <typename ExecContext>
concept execution_context = is_execution_context_v<ExecContext>;

template <typename Exec>
concept schedulable = is_schedulable_v<Exec>;

template <typename Func>
concept awaitable_function =
	is_functor_v<Func> and
	is_awaitable_v<typename function_traits<Func>::return_type> and
	function_traits<Func>::arg_count == 0;

}} //namespace libgs::concepts


#endif //LIBGS_CORE_CXX_ASIO_CONCEPT_H
