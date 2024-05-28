
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
	close(error);
}

template <concept_execution Exec, typename Derived>
basic_tcp_socket<Exec,Derived>::basic_tcp_socket(basic_tcp_socket &&other) noexcept :
	base_t(std::move(other)),
	m_resolver(std::move(other.m_resolver))
{
	other.m_native = new native_t(this->executor());
}

template <concept_execution Exec, typename Derived>
basic_tcp_socket<Exec,Derived> &basic_tcp_socket<Exec,Derived>::operator=(basic_tcp_socket &&other) noexcept
{
	base_t::operator=(std::move(other));
	other.m_native = new native_t(this->executor());
	m_resolver = std::move(other.m_resolver);
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
bool basic_tcp_socket<Exec,Derived>::is_open() const noexcept
{
	return native().is_open();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::derived_t &basic_tcp_socket<Exec,Derived>::close(no_time_token tk)
{
	error_code error;
	native().close(error);
	detail::check_error(tk.error, error, "libgs::io::tcp_socket::close");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::derived_t &basic_tcp_socket<Exec,Derived>::cancel() noexcept
{
	this->m_write_cancel = true;
	this->m_read_cancel = true;

	this->m_connect_cancel = true;
	this->m_dns_cancel = true;

	m_resolver.cancel();
	native().cancel();
	return this->derived();
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_tcp_socket<Exec,Derived>::remote_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = native().remote_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::tcp_socket::remote_endpoint");
	return {ep.address(), ep.port()};
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_tcp_socket<Exec,Derived>::local_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = native().local_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::Tcp_socket::local_endpoint");
	return {ep.address(), ep.port()};
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::derived_t &basic_tcp_socket<Exec,Derived>::set_option
(const socket_option &op, no_time_token tk)
{
	using namespace asio;
	error_code error;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		native().set_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		native().set_option(*reinterpret_cast<ip::v6_only*>(op.data), error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
	detail::check_error(tk.error, error, "libgs::io::tcp_socket::local_endpoint");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_socket<Exec,Derived>::derived_t &basic_tcp_socket<Exec,Derived>::get_option
(socket_option op, no_time_token tk)
{
	using namespace asio;
	error_code error;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		native().get_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		native().get_option(*reinterpret_cast<ip::v6_only*>(op.data), error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
	detail::check_error(tk.error, error, "libgs::io::tcp_socket::local_endpoint");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
const typename basic_tcp_socket<Exec,Derived>::derived_t &basic_tcp_socket<Exec,Derived>::get_option
(socket_option op, no_time_token tk) const
{
	return remove_const(this)->get_option(op,tk);
}

template <concept_execution Exec, typename Derived>
size_t basic_tcp_socket<Exec,Derived>::read_buffer_size() const noexcept
{
	asio::socket_base::receive_buffer_size op;
	error_code error;
	get_option(op, error);
	return op.value();
}

template <concept_execution Exec, typename Derived>
size_t basic_tcp_socket<Exec,Derived>::write_buffer_size() const noexcept
{
	asio::socket_base::send_buffer_size op;
	error_code error;
	get_option(op, error);
	return op.value();
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
void basic_tcp_socket<Exec,Derived>::_delete_native(void *native)
{
	delete reinterpret_cast<native_t*>(native);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SOCKET_H
