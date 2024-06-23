
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

#ifndef LIBGS_IO_DETAIL_TCP_SOCKET_H
#define LIBGS_IO_DETAIL_TCP_SOCKET_H

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_tcp_socket<Exec,Derived>::basic_tcp_socket(Context &context) :
	base_t(new native_t(context), context.get_executor()),
	m_resolver(context)
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_socket<Exec,Derived>::basic_tcp_socket(asio_basic_tcp_socket<Exec0> &&native) :
	base_t(reinterpret_cast<void*>(1), native.get_executor()),
	m_resolver(native.get_executor())
{
	this->m_native = new native_t(std::move(native));
}

template <concept_execution Exec, typename Derived>
basic_tcp_socket<Exec,Derived>::basic_tcp_socket(const executor_t &exec) :
	base_t(new native_t(exec), exec),
	m_resolver(exec)
{

}

template <concept_execution Exec, typename Derived>
basic_tcp_socket<Exec,Derived>::~basic_tcp_socket()
{
	error_code error;
	native().close(error);
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_socket<Exec,Derived>::basic_tcp_socket(basic_tcp_socket<Exec0,Derived> &&other) noexcept :
	base_t(std::move(other)),
	m_resolver(std::move(other.m_resolver))
{
	this->m_native = new native_t(std::move(other.native()));
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_socket<Exec,Derived> &basic_tcp_socket<Exec,Derived>::operator=(basic_tcp_socket<Exec0,Derived> &&other) noexcept
{
	base_t::operator=(std::move(other));
	m_resolver = std::move(other.m_resolver);
	native() = std::move(other.native());
	return *this;
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_socket<Exec,Derived> &basic_tcp_socket<Exec,Derived>::operator=(asio_basic_tcp_socket<Exec0> &&native) noexcept
{
	this->native() = std::move(native);
	return *this;
}

template <concept_execution Exec, typename Derived>
const typename basic_tcp_socket<Exec,Derived>::native_t &basic_tcp_socket<Exec,Derived>::native() const noexcept
{
	return *reinterpret_cast<const native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::native_t &basic_tcp_socket<Exec,Derived>::native() noexcept
{
	return *reinterpret_cast<native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
const typename basic_tcp_socket<Exec,Derived>::resolver_t &basic_tcp_socket<Exec,Derived>::resolver() const noexcept
{
	return m_resolver;
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::resolver_t &basic_tcp_socket<Exec,Derived>::resolver() noexcept
{
	return m_resolver;
}

template <concept_execution Exec, typename Derived>
void basic_tcp_socket<Exec,Derived>::_cancel() noexcept
{
	base_t::_cancel();
	this->m_dns_cancel = true;
	m_resolver.cancel();
}

template <concept_execution Exec, typename Derived>
void basic_tcp_socket<Exec,Derived>::_delete_native(void *native)
{
	delete reinterpret_cast<native_t*>(native);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SOCKET_H
