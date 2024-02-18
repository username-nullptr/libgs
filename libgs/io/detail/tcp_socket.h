
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

template <concept_execution Exec>
template <concept_execution_context Context>
basic_tcp_socket<Exec>::basic_tcp_socket(Context &context) :
	base_type(context.get_executor()),
	m_sock(std::make_shared<asio_socket>(context)),
	m_resolver(context)
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_tcp_socket<Exec>::basic_tcp_socket(asio_basic_tcp_socket<Exec0> &&sock) :
	base_type(sock.get_executor()),
	m_sock(std::make_shared<asio_socket>(std::move(sock))),
	m_resolver(m_sock->get_executor())
{

}

template <concept_execution Exec>
basic_tcp_socket<Exec>::basic_tcp_socket(const executor_type &exec) :
	base_type(exec),
	m_sock(std::make_shared<asio_socket>(exec)),
	m_resolver(m_sock->get_executor())
{

}

template <concept_execution Exec>
basic_tcp_socket<Exec>::~basic_tcp_socket()
{
	error_code error;
	m_sock->shutdown(shutdown_type::shutdown_both, error);
	m_sock->close(error);
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::connect(endpoint ep, error_code &error) noexcept
{
	m_sock->connect({ep.addr, ep.port}, error);
}

template <concept_execution Exec>
typename basic_tcp_socket<Exec>::address_vector basic_tcp_socket<Exec>::dns(std::string domain, error_code &error) noexcept
{
	address_vector vector;
	auto results = m_resolver.resolve(domain, "0", error);

	if( error )
		return vector;

	for(auto &addr : vector)
		vector.emplace_back(std::move(addr));
	return vector;
}

template <concept_execution Exec>
size_t basic_tcp_socket<Exec>::read(buffer<void*> buf, error_code &error) noexcept
{
	return m_sock->read_some(asio::buffer(buf.data, buf.size), error);
}

template <concept_execution Exec>
size_t basic_tcp_socket<Exec>::write(buffer<const void*> buf, error_code &error) noexcept
{
	return m_sock->write_some(asio::buffer(buf.data, buf.size), error);
}

template <concept_execution Exec>
basic_tcp_socket<Exec>::endpoint basic_tcp_socket<Exec>::remote_endpoint(error_code &error) const noexcept
{
	auto ep = m_sock->remote_endpoint(error);
	return {ep.address(), ep.port()};
}

template <concept_execution Exec>
basic_tcp_socket<Exec>::endpoint basic_tcp_socket<Exec>::local_endpoint(error_code &error) const noexcept
{
	auto ep = m_sock->local_endpoint(error);
	return {ep.address(), ep.port()};
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::shutdown(error_code &error, shutdown_type what) noexcept
{
	m_sock->shutdown(what, error);
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::close(error_code &error) noexcept
{
	m_sock->close(error);
}

template <concept_execution Exec>
bool basic_tcp_socket<Exec>::is_open() const noexcept
{
	return m_sock->is_open();
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::cancel() noexcept
{
	m_write_cancel = true;
	m_read_cancel = true;

	m_connect_cancel = true;
	m_dns_cancel = true;

	m_resolver.cancel();
	m_sock->cancel();
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::set_option(const option &op, error_code &error) noexcept
{
	using namespace asio;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		m_sock->set_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		m_sock->set_option(*reinterpret_cast<ip::v6_only*>(op.data), error);
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::get_option(option op, error_code &error) const noexcept
{
	using namespace asio;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		m_sock->get_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		m_sock->get_option(*reinterpret_cast<ip::v6_only*>(op.data), error);
}

template <concept_execution Exec>
typename basic_tcp_socket<Exec>::asio_socket &basic_tcp_socket<Exec>::native_object() const
{
	return *m_sock;
}

template <concept_execution Exec>
tcp_handle_type basic_tcp_socket<Exec>::native_handle() const
{
	return m_sock->native_handle();
}

template <concept_execution Exec>
awaitable<error_code> basic_tcp_socket<Exec>::do_connect(const endpoint &ep) noexcept 
{
	error_code error;
	m_connect_cancel = false;
	co_await m_sock->async_connect({ep.addr, ep.port}, use_awaitable_e[error]);

	if( m_connect_cancel )
	{
		error = errc::operation_aborted;
		m_connect_cancel = false;
	}
	co_return error;
}

template <concept_execution Exec>
awaitable<typename basic_tcp_socket<Exec>::address_vector> basic_tcp_socket<Exec>::do_dns
(const std::string &domain, error_code &error) noexcept 
{
	address_vector vector;

	m_dns_cancel = false;
	auto results = co_await m_resolver.async_resolve(domain, "0", use_awaitable_e[error]);

	if( m_dns_cancel )
	{
		error = errc::operation_aborted;
		m_dns_cancel = false;
		co_return vector;
	}
	else if( error )
		co_return vector;

	for(auto &ep : results)
		vector.emplace_back(ep.endpoint().address());
	co_return vector;
}

template <concept_execution Exec>
awaitable<size_t> basic_tcp_socket<Exec>::read_data(void *buf, size_t size, error_code &error) noexcept 
{
	m_read_cancel = false;
	size = co_await m_sock->async_read_some(asio::buffer(buf, size), use_awaitable_e[error]);

	if( m_read_cancel )
	{
		error = errc::operation_aborted;
		m_read_cancel = false;
	}
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_tcp_socket<Exec>::write_data(const void *buf, size_t size, error_code &error) noexcept 
{
	m_write_cancel = false;
	size = co_await m_sock->async_write_some(asio::buffer(buf, size), use_awaitable_e[error]);

	if( m_write_cancel )
	{
		error = errc::operation_aborted;
		m_write_cancel = false;
	}
	co_return size;
}

template <concept_execution Exec, typename...Args>
basic_tcp_socket_ptr<Exec> make_basic_tcp_socket(Args&&...args)
{
	return std::make_shared<basic_tcp_socket<Exec>>(std::forward<Args>(args)...);
}

template <typename...Args>
tcp_socket_ptr make_tcp_socket(Args&&...args)
{
	return std::make_shared<tcp_socket>(std::forward<Args>(args)...);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SOCKET_H
