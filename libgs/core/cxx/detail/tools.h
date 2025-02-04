
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

#ifndef LIBGS_CORE_CXX_DETAIL_TOOLS_H
#define LIBGS_CORE_CXX_DETAIL_TOOLS_H

namespace libgs
{

template <typename T>
const char *type_name()
{
	return LIBGS_ABI_CXA_DEMANGLE(typeid(T).name());
}

const char *type_name(auto &&t)
{
	return LIBGS_ABI_CXA_DEMANGLE(typeid(t).name());
}

template <typename T>
constexpr T &remove_const(const T &v)
{
	return const_cast<T&>(v);
}

template <typename T>
constexpr T *remove_const(const T *v)
{
	return const_cast<T*>(v);
}

template <typename T>
constexpr const T &as_const(const T &v)
{
	return v;
}

template <typename T>
constexpr const T &&as_const(const T &&v)
{
	return std::move(v);
}

template <typename T>
constexpr const T *as_const(const T *v)
{
	return v;
}

decltype(auto) get_executor_helper(concepts::schedulable auto &&exec)
{
	using Exec = decltype(exec);
	using exec_t = std::remove_cvref_t<Exec>;

	if constexpr( is_execution_v<exec_t> )
		return std::forward<Exec>(exec);
	else
		return exec.get_executor();
}

decltype(auto) unbound_token(concepts::any_tf_opt_token auto &&token)
{
	using Token = decltype(token);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_redirect_time_v<token_t> )
		return unbound_token(token.token);
	else if constexpr( is_redirect_error_v<token_t> )
		return return_reference(token.token_);
	else if constexpr( is_cancellation_slot_binder_v<token_t> )
		return token.get();
	else
		return std::forward<Token>(token);
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_TOOLS_H