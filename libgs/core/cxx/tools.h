
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

#ifndef LIBGS_CORE_CXX_TOOLS_H
#define LIBGS_CORE_CXX_TOOLS_H

#include <libgs/core/cxx/token_concepts.h>
#include <libgs/core/cxx/remove_repeat.h>
#include <libgs/core/cxx/string_tools.h>
#include <libgs/core/cxx/type_traits.h>

#ifdef __GNUC__
# include <cxxabi.h>
# define LIBGS_ABI_CXA_DEMANGLE(name)  abi::__cxa_demangle(name, nullptr, nullptr, nullptr)
#else //_MSVC
# define LIBGS_ABI_CXA_DEMANGLE(name)  name
#endif //__GNUC__

namespace libgs
{

using std_type_id = decltype(typeid(void).hash_code());

template <typename T>
[[nodiscard]] LIBGS_CORE_TAPI const char *type_name();
[[nodiscard]] LIBGS_CORE_TAPI const char *type_name(auto &&t);

template <typename T>
[[nodiscard]] constexpr T &remove_const(const T &v);

template <typename T>
[[nodiscard]] constexpr T *remove_const(const T *v);

template <typename T>
[[nodiscard]] constexpr const T &as_const(const T &v);

template <typename T>
[[nodiscard]] constexpr const T *as_const(const T *v);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) get_executor_helper (
	concepts::schedulable auto &&exec
);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) unbound_token (
	concepts::any_tf_opt_token auto &&token
);

template <typename...Args>
constexpr void ignore_unused(Args&&...) {}

template <concepts::any_tf_opt_token Token>
struct token_unbound
{
	using type = std::remove_cvref_t <
		decltype(unbound_token(std::declval<Token>()))
	>;
};

template <concepts::any_tf_opt_token Token>
using token_unbound_t = typename token_unbound<Token>::type;

template <typename T, typename U> requires std::is_class_v<T>
struct class_member
{
	using type = std::remove_reference_t <
		decltype(std::declval<T>().*std::declval<U>())
	>;
};

template <typename T, typename U> requires std::is_class_v<T>
using class_member_t = typename class_member<T,U>::type;

template <typename Derived, typename Base>
struct crtp_derived { using type = Derived; };

template <typename Base>
struct crtp_derived<void,Base> { using type = Base; };

template <typename Derived, typename Base>
using crtp_derived_t = typename crtp_derived<Derived, Base>::type;

} //namespace libgs
#include <libgs/core/cxx/detail/tools.h>


#endif //LIBGS_CORE_CXX_TOOLS_H