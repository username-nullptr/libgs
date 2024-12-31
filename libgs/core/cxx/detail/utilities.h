
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

#ifndef LIBGS_CORE_CXX_DETAIL_UTILITIES_H
#define LIBGS_CORE_CXX_DETAIL_UTILITIES_H

#include <memory>

#ifdef _MSC_VER
# include <cstdlib>
#endif

namespace libgs
{

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
constexpr const T *as_const(const T *v)
{
	return v;
}

template <typename T>
const char *type_name()
{
	return LIBGS_ABI_CXA_DEMANGLE(typeid(T).name());
}

const char *type_name(auto &&t)
{
	return LIBGS_ABI_CXA_DEMANGLE(typeid(t).name());
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

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::basic_endpoint_wrapper(string_wrapper address, uint16_t port) :
	value(asio::ip::address::from_string(address.value), port)
{

}

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::basic_endpoint_wrapper(string_wrapper address) :
	basic_endpoint_wrapper(std::move(address), 0)
{

}

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::basic_endpoint_wrapper(ip_type type, uint16_t port)
{
	if( type == ip_type::v4 )
		value = endpoint_t(asio::ip::address_v4::any(), port);
	else if( type == ip_type::v6 )
		value = endpoint_t(asio::ip::address_v6::any(), port);
	else
		value = endpoint_t(asio::ip::address_v4::loopback(), port);
}

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::basic_endpoint_wrapper(ip_type type) :
	basic_endpoint_wrapper(type, 0)
{
}

template <typename Protocol>
template <typename...Args>
basic_endpoint_wrapper<Protocol>::basic_endpoint_wrapper(Args&&...args) requires
	concepts::constructible<endpoint_t,Args&&...>
{
	if constexpr( sizeof...(Args) == 2 )
	{
		auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
		value = endpoint_t(std::get<0>(tuple), static_cast<uint16_t>(std::get<1>(tuple)));
	}
	else
		value = endpoint_t(std::forward<Args>(args)...);
}

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::operator endpoint_t&()
{
	return value;
}

template <typename Protocol>
basic_endpoint_wrapper<Protocol>::operator const endpoint_t&() const
{
	return value;
}

template <typename Protocol>
typename basic_endpoint_wrapper<Protocol>::endpoint_t &basic_endpoint_wrapper<Protocol>::operator*()
{
	return value;
}

template <typename Protocol>
typename basic_endpoint_wrapper<Protocol>::endpoint_t *basic_endpoint_wrapper<Protocol>::operator->()
{
	return &value;
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_UTILITIES_H
