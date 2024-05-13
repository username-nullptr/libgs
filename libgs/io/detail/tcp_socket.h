
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
	base_type(new asio_socket_type(context), [this]
	{
		error_code error;
		native_object().shutdown(shutdown_type::shutdown_both, error);
		native_object().close(error);
	}),
	m_resolver(context)
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_tcp_socket<Exec>::basic_tcp_socket(asio_basic_tcp_socket<Exec0> &&sock) :
	base_type(new asio_socket_type(std::move(sock)), [this]
	{
		error_code error;
		native_object().shutdown(shutdown_type::shutdown_both, error);
		native_object().close(error);
	}),
	m_resolver(reinterpret_cast<asio_socket_type*>(this->m_sock)->get_executor())
{

}

template <concept_execution Exec>
basic_tcp_socket<Exec>::basic_tcp_socket(const executor_type &exec) :
	base_type(new asio_socket_type(exec), [this]
	{
		error_code error;
		native_object().shutdown(shutdown_type::shutdown_both, error);
		native_object().close(error);
	}),
	m_resolver(reinterpret_cast<asio_socket_type*>(this->m_sock)->get_executor())
{

}

template <concept_execution Exec>
ip_endpoint basic_tcp_socket<Exec>::remote_endpoint(error_code &error) const noexcept
{
	auto ep = native_object().remote_endpoint(error);
	return {ep.address(), ep.port()};
}

template <concept_execution Exec>
ip_endpoint basic_tcp_socket<Exec>::local_endpoint(error_code &error) const noexcept
{
	auto ep = native_object().local_endpoint(error);
	return {ep.address(), ep.port()};
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::shutdown(error_code &error, shutdown_type what) noexcept
{
	native_object().shutdown(what, error);
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::close(error_code &error) noexcept
{
	auto &sock = native_object();
	sock.non_blocking(false);
	sock.close(error);
}

template <concept_execution Exec>
bool basic_tcp_socket<Exec>::is_open() const noexcept
{
	return native_object().is_open();
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::cancel() noexcept
{
	m_write_cancel = true;
	m_read_cancel = true;

	m_connect_cancel = true;
	m_dns_cancel = true;

	m_resolver.cancel();
	native_object().cancel();
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::set_option(const socket_option &op, error_code &error) noexcept
{
	using namespace asio;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		native_object().set_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		native_object().set_option(*reinterpret_cast<ip::v6_only*>(op.data), error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
}

template <concept_execution Exec>
void basic_tcp_socket<Exec>::get_option(socket_option op, error_code &error) const noexcept
{
	using namespace asio;

	if( op.id == typeid(socket_base::broadcast).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::broadcast*>(op.data), error);

	else if( op.id == typeid(socket_base::debug).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::debug*>(op.data), error);
	
	else if( op.id == typeid(socket_base::do_not_route).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::do_not_route*>(op.data), error);
	
	else if( op.id == typeid(socket_base::enable_connection_aborted).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::enable_connection_aborted*>(op.data), error);
	
	else if( op.id == typeid(socket_base::keep_alive).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::keep_alive*>(op.data), error);
	
	else if( op.id == typeid(socket_base::linger).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::linger*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_buffer_size).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::receive_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::receive_low_watermark).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::receive_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(socket_base::reuse_address).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::reuse_address*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_buffer_size).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::send_buffer_size*>(op.data), error);
	
	else if( op.id == typeid(socket_base::send_low_watermark).hash_code() )
		native_object().get_option(*reinterpret_cast<socket_base::send_low_watermark*>(op.data), error);
	
	else if( op.id == typeid(ip::v6_only).hash_code() )
		native_object().get_option(*reinterpret_cast<ip::v6_only*>(op.data), error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
}

template <concept_execution Exec>
typename basic_tcp_socket<Exec>::asio_socket_type &basic_tcp_socket<Exec>::native_object()
{
	return *reinterpret_cast<asio_socket_type*>(this->m_handle);
}

template <concept_execution Exec>
const typename basic_tcp_socket<Exec>::asio_socket_type &basic_tcp_socket<Exec>::native_object() const
{
	return remove_const(this)->native_object();
}

template <concept_execution Exec>
tcp_handle_type basic_tcp_socket<Exec>::native_handle() const
{
	return native_object().native_handle();
}

template <concept_execution Exec>
awaitable<error_code> basic_tcp_socket<Exec>::do_connect(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept
{
	error_code error;
	m_connect_cancel = false;

	if( cnl_sig )
	{
		co_await native_object()
		.async_connect({ep.addr, ep.port}, asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
		co_await native_object().async_connect({ep.addr, ep.port}, use_awaitable_e[error]);

	if( m_connect_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_connect_cancel = false;
	}
	co_return error;
}

template <concept_execution Exec>
awaitable<typename basic_tcp_socket<Exec>::address_vector> basic_tcp_socket<Exec>::do_dns
(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	typename resolver::results_type results;
	address_vector vector;
	m_dns_cancel = false;

	if( cnl_sig )
	{
		results = co_await m_resolver
				.async_resolve(domain, "0", asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
		results = co_await m_resolver.async_resolve(domain, "0", use_awaitable_e[error]);

	if( m_dns_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
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
awaitable<size_t> basic_tcp_socket<Exec>::read_data(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	m_read_cancel = false;
	if( rc.var.index() == 0 )
	{
		auto delim = std::get<0>(rc.var);
		if( delim.empty() )
		{
			if( cnl_sig )
			{
				buf.size = co_await native_object()
						.async_read_some(asio::buffer(buf.data, buf.size),
										 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await native_object()
						.async_read_some(asio::buffer(buf.data, buf.size), use_awaitable_e[error]);
			}
		}
		else
		{
			std::string _buf;
			if( cnl_sig )
			{
				buf.size = co_await asio::async_read_until
						(native_object(), asio::dynamic_buffer(_buf, buf.size), delim,
						 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await asio::async_read_until
						(native_object(), asio::dynamic_buffer(_buf, buf.size), delim, use_awaitable_e[error]);
			}
			if( buf.size > 0 )
				memcpy(buf.data, _buf.c_str(), buf.size);
		}
	}
	else
	{
		std::string _buf;
		if( cnl_sig )
		{
			buf.size = co_await asio::async_read_until
					(native_object(), asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var),
					 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
		}
		else
		{
			buf.size = co_await asio::async_read_until
					(native_object(), asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var), use_awaitable_e[error]);
		}
		if( buf.size > 0 )
			memcpy(buf.data, _buf.c_str(), buf.size);
	}
	if( m_read_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_read_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec>
awaitable<size_t> basic_tcp_socket<Exec>::write_data(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	m_write_cancel = false;
	if( cnl_sig )
	{
		buf.size = co_await native_object().async_write_some
				(asio::buffer(buf.data.data(), buf.size), asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
		buf.size = co_await native_object().async_write_some(asio::buffer(buf.data.data(), buf.size), use_awaitable_e[error]);

	if( m_write_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_write_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec>
basic_tcp_socket<Exec>::basic_tcp_socket(auto *asio_sock, concept_callable auto &&del_sock) : 
	base_type(asio_sock, std::forward<decltype(del_sock)>(del_sock)),
	m_resolver(asio_sock->get_executor())
{

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
