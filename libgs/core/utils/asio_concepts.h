
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

#ifndef LIBGS_CORE_UTILS_ASIO_CONCEPTS_H
#define LIBGS_CORE_UTILS_ASIO_CONCEPTS_H

#include <libgs/core/cxx/concepts.h>

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
struct is_exec
{
	static constexpr bool value =
		(asio::execution::is_executor<Exec>::value and
		 asio::can_require<Exec, asio::execution::blocking_t::never_t>::value) or
		 asio::is_executor<Exec>::value;
};

template <typename Exec>
constexpr bool is_exec_v = is_exec<Exec>::value;

template <typename ExecContext>
struct is_exec_context {
	static constexpr bool value = asio::is_convertible<ExecContext&, asio::execution_context&>::value;
};

template <typename ExecContext>
constexpr bool is_exec_context_v = is_exec_context<ExecContext>::value;

template <typename Exec>
struct is_sched {
	static constexpr bool value = is_exec_v<Exec> or is_exec_context_v<Exec>;
};

template <typename Exec>
constexpr bool is_sched_v = is_sched<Exec>::value;

namespace concepts
{

template <typename Exec, typename NativeExec>
concept match_exec =
	is_exec_v<Exec> and is_exec_v<NativeExec> and
	requires(const Exec &exec) { NativeExec(exec); };

template <typename Exec, typename NativeExec>
concept match_exec_context =
	is_exec_context_v<Exec> and is_exec_v<NativeExec> and std::is_lvalue_reference_v<Exec&> and
	requires(Exec &exec) { NativeExec(exec.get_executor()); };

template <typename Exec, typename NativeExec>
concept match_sched =
	match_exec<Exec,NativeExec> or
	match_exec_context<Exec,NativeExec>;

template <typename Exec>
concept exec = is_exec_v<Exec>;

template <typename ExecContext>
concept exec_context = is_exec_context_v<ExecContext>;

template <typename Exec>
concept sched = exec<Exec> or (
	exec_context<Exec> and std::is_lvalue_reference_v<Exec> and not std::is_const_v<Exec>
);

} //namespace concepts

template <typename Exec, typename NativeExec>
struct is_match_exec {
	static constexpr bool value = concepts::match_exec<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_exec_v = is_match_exec<Exec,NativeExec>::value;

template <typename Exec, typename NativeExec>
struct is_match_exec_context {
	static constexpr bool value = concepts::match_exec_context<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_exec_context_v = is_match_exec_context<Exec,NativeExec>::value;

template <typename Exec, typename NativeExec>
struct is_match_sched {
	static constexpr bool value = concepts::match_sched<Exec,NativeExec>;
};

template <typename Exec, typename NativeExec>
constexpr bool is_match_sched_v = is_match_sched<Exec,NativeExec>::value;

template <typename T>
using awaitable = asio::awaitable<T>;

template <typename>
struct is_awaitable : std::false_type {};

template <typename T>
struct is_awaitable<awaitable<T>> : std::true_type {};

template <typename T>
constexpr bool is_awaitable_v = is_awaitable<T>::value;

namespace concepts
{

template <typename T>
concept awaitable = is_awaitable_v<T>;

template <typename T>
concept awaitable_p = std::is_rvalue_reference_v<T> and awaitable<std::remove_cvref_t<T>>;

template <typename Func>
concept awaitable_func =
	is_functor_v<Func> and
	is_awaitable_v<typename function_traits<Func>::return_type> and
	function_traits<Func>::arg_count == 0;

} //namespace concepts

template <typename>
struct awaitable_ret {};

template <typename T>
struct awaitable_ret<asio::awaitable<T>> {
	using type = T;
};

template <concepts::awaitable T>
using awaitable_ret_t = awaitable_ret<T>::type;

namespace concepts
{

template <typename Func, typename RT>
concept awaitable_ret_func =
	awaitable_func<Func> and
	std::is_same_v <
		awaitable_ret_t <
			typename function_traits<Func>::return_type
		>, RT
	>;

template <typename Func>
concept awaitable_void_func = awaitable_ret_func<Func,void>;

} //namespace concepts

} //namespace libgs::concepts


#endif //LIBGS_CORE_UTILS_ASIO_CONCEPTS_H
