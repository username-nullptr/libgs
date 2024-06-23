
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

#ifndef LIBGS_IO_DETAIL_UDP_SOCKET_H
#define LIBGS_IO_DETAIL_UDP_SOCKET_H

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_udp_socket<Exec,Derived>::basic_udp_socket(Context &context) :
	base_t(new native_t(context), context.get_executor()),
	m_resolver(context)
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_udp_socket<Exec,Derived>::basic_udp_socket(asio_basic_udp_socket<Exec0> &&native) :
	base_t(reinterpret_cast<void*>(1), native.get_executor()),
	m_resolver(native.get_executor())
{
	this->m_native = new native_t(std::move(native));
}

template <concept_execution Exec, typename Derived>
basic_udp_socket<Exec,Derived>::basic_udp_socket(const executor_t &exec) :
	base_t(new native_t(exec), exec),
	m_resolver(exec)
{

}

template <concept_execution Exec, typename Derived>
basic_udp_socket<Exec,Derived>::~basic_udp_socket()
{
	error_code error;
	native().close(error);
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_udp_socket<Exec,Derived>::basic_udp_socket(basic_udp_socket<Exec0,Derived> &&other) noexcept :
	base_t(std::move(other)),
	m_resolver(std::move(other.m_resolver))
{
	this->m_native = new native_t(std::move(other.native()));
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_udp_socket<Exec,Derived> &basic_udp_socket<Exec,Derived>::operator=(basic_udp_socket<Exec0,Derived> &&other) noexcept
{
	base_t::operator=(std::move(other));
	m_resolver = std::move(other.m_resolver);
	native() = std::move(other.native());
	return *this;
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_udp_socket<Exec,Derived> &basic_udp_socket<Exec,Derived>::operator=(asio_basic_udp_socket<Exec0> &&native) noexcept
{
	this->native() = std::move(native);
	return *this;
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::read
(host_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::read
(host_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::read
(host_endpoint ep, buffer<void*> buf, opt_token<error_code&> tk)
{

}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::read
(host_endpoint ep, buffer<std::string&> buf, opt_token<error_code&> tk)
{

}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::read
(ip_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::read
(ip_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::read
(ip_endpoint ep, buffer<void*> buf, opt_token<error_code&> tk)
{

}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::read
(ip_endpoint ep, buffer<std::string&> buf, opt_token<error_code&> tk)
{

}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::write
(host_endpoint ep, buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::write
(host_endpoint ep, buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::derived_t &basic_udp_socket<Exec,Derived>::write
(ip_endpoint ep, buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::write
(ip_endpoint ep, buffer<std::string_view> buf, opt_token<error_code&> tk)
{

}

template <concept_execution Exec, typename Derived>
const typename basic_udp_socket<Exec,Derived>::native_t &basic_udp_socket<Exec,Derived>::native() const noexcept
{
	return *reinterpret_cast<const native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::native_t &basic_udp_socket<Exec,Derived>::native() noexcept
{
	return *reinterpret_cast<native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
const typename basic_udp_socket<Exec,Derived>::resolver_t &basic_udp_socket<Exec,Derived>::resolver() const noexcept
{
	return m_resolver;
}

template <concept_execution Exec, typename Derived>
typename basic_udp_socket<Exec,Derived>::resolver_t &basic_udp_socket<Exec,Derived>::resolver() noexcept
{
	return m_resolver;
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::_read_data
(buffer<void*> buf, read_condition, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	this->m_read_cancel = false;
	if( cnl_sig )
	{
		buf.size = co_await this->derived().native()
				.async_receive(asio::buffer(buf.data, buf.size),
							   asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		buf.size = co_await this->derived().native()
				.async_receive(asio::buffer(buf.data, buf.size), use_awaitable_e[error]);
	}
	if( this->m_read_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_read_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec, typename Derived>
awaitable<size_t> basic_udp_socket<Exec,Derived>::_write_data
(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	this->m_write_cancel = false;
	if( cnl_sig )
	{
		buf.size = co_await this->derived().native().async_send
				(asio::buffer(buf.data.data(), buf.size),
				 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		buf.size = co_await this->derived().native().async_send
				(asio::buffer(buf.data.data(), buf.size), use_awaitable_e[error]);
	}
	if( this->m_write_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_write_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec, typename Derived>
void basic_udp_socket<Exec,Derived>::_cancel() noexcept
{
	base_t::_cancel();
	this->m_dns_cancel = true;
	m_resolver.cancel();
}

template <concept_execution Exec, typename Derived>
void basic_udp_socket<Exec,Derived>::_delete_native(void *native)
{
	delete reinterpret_cast<native_t*>(native);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_UDP_SOCKET_H
