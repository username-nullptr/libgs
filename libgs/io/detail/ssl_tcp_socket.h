
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

#ifndef LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H
#define LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H

#ifdef LIBGS_ENABLE_OPENSSL

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(Context &context, ssl::context &ssl, handshake_t type) :
	base_t(new ssl_stream_t(context, ssl), context.get_executor()), m_type(type)
{

}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(const executor_t &exec, ssl::context &ssl, handshake_t type) :
	base_t(new ssl_stream_t(exec, ssl), exec), m_type(type)
{

}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(ssl::context &ssl, handshake_t type) :
	base_t(new ssl_stream_t(ssl), execution::io_context().get_executor()), m_type(type)
{

}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket
(auto next_layer, ssl::context &ssl, handshake_t type) requires requires
{
	ssl_stream_t(std::move(next_layer.native()), ssl);
	tcp_socket_t(std::move(next_layer));
}:
	base_t(reinterpret_cast<void*>(1), next_layer.get_executor()), m_type(type)
{
	this->m_native = new ssl_stream_t(std::move(next_layer), ssl);
}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(ssl_stream_t &&native, handshake_t type) :
	base_t(reinterpret_cast<void*>(1), native.get_executor()), m_type(type)
{
	this->m_native = new ssl_stream_t(std::move(native));
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(basic_ssl_tcp_socket<Exec0,Derived> &&other) noexcept :
	base_t(std::move(other)),
	m_type(other.m_type)
{
	this->m_native = new ssl_stream_t(std::move(other.ssl_stream()));
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_socket<Exec,Derived> &basic_ssl_tcp_socket<Exec,Derived>::operator=(basic_ssl_tcp_socket<Exec0,Derived> &&other) noexcept
{
	base_t::operator=(std::move(other));
	ssl_stream() = std::move(other.ssl_stream());
	m_type = other.m_type;
	return *this;
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_socket<Exec,Derived> &basic_ssl_tcp_socket<Exec,Derived>::operator=(other_ssl_stream_t<Exec0> &&native) noexcept
{
	ssl_stream() = std::move(native);
	return *this;
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_ssl_tcp_socket<Exec,Derived>::remote_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = ssl_stream().next_layer().remote_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::remote_endpoint");
	return ep;
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_ssl_tcp_socket<Exec,Derived>::local_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = ssl_stream().next_layer().local_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::local_endpoint");
	return ep;
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::set_option(const socket_option &op, no_time_token tk)
{
	error_code error;
	ssl_stream().next_layer().set_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::set_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::get_option(socket_option op, no_time_token tk)
{
	error_code error;
	ssl_stream().next_layer().get_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::get_option(socket_option op, no_time_token tk) const
{
	error_code error;
	ssl_stream().next_layer().get_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
size_t basic_ssl_tcp_socket<Exec,Derived>::read_buffer_size() const noexcept
{
	return ssl_stream().read_buffer_size();
}

template <concept_execution Exec, typename Derived>
size_t basic_ssl_tcp_socket<Exec,Derived>::write_buffer_size() const noexcept
{
	return ssl_stream().read_buffer_size();
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::ssl_stream_t&
basic_ssl_tcp_socket<Exec,Derived>::ssl_stream() const noexcept
{
	return *reinterpret_cast<const ssl_stream_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::ssl_stream_t&
basic_ssl_tcp_socket<Exec,Derived>::ssl_stream() noexcept
{
	return *reinterpret_cast<ssl_stream_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::native_t &basic_ssl_tcp_socket<Exec,Derived>::native() const noexcept
{
	return ssl_stream().native();
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::native_t &basic_ssl_tcp_socket<Exec,Derived>::native() noexcept
{
	return ssl_stream().native();
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::resolver_t &basic_ssl_tcp_socket<Exec,Derived>::resolver() const noexcept
{
	return ssl_stream().next_layer().resolver();
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::resolver_t &basic_ssl_tcp_socket<Exec,Derived>::resolver() noexcept
{
	return ssl_stream().next_layer().resolver();
}

template <concept_execution Exec, typename Derived>
awaitable<error_code> basic_ssl_tcp_socket<Exec,Derived>::_connect
(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept
{
	error_code error;
	if( m_type == handshake_t::server )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::no_permission));
		co_return error;
	}
	this->m_connect_cancel = false;
	if( cnl_sig )
		co_await ssl_stream().next_layer().connect(ep, {*cnl_sig, error});
	else
		co_await ssl_stream().next_layer().connect(ep, error);

	if( not error and not this->m_connect_cancel )
	{
		if( cnl_sig )
			co_await ssl_stream().handshake(m_type, {*cnl_sig, error});
		else
			co_await ssl_stream().handshake(m_type, error);
	}
	if( this->m_connect_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_connect_cancel = false;
	}
	co_return error;
}

template <concept_execution Exec, typename Derived>
awaitable<error_code> basic_ssl_tcp_socket<Exec,Derived>::_close(cancellation_signal *cnl_sig)
{
	error_code error;
	if( cnl_sig )
		co_await ssl_stream().wave({*cnl_sig, error});
	else
		co_await ssl_stream().wave(error);

	if( not error )
		co_await ssl_stream().next_layer().close(error);
	co_return error;
}

template <concept_execution Exec, typename Derived>
void basic_ssl_tcp_socket<Exec,Derived>::_cancel() noexcept
{
	base_t::_cancel();
	ssl_stream().cancel();
}

template <concept_execution Exec, typename Derived>
void basic_ssl_tcp_socket<Exec,Derived>::_delete_native(void *native)
{
	delete reinterpret_cast<ssl_stream_t*>(native);
}

} //namespace libgs::io

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H
