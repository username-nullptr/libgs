
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

namespace libgs::io
{

template <concept_execution Exec>
template <concept_execution_context Context>
basic_ssl_tcp_socket<Exec>::basic_ssl_tcp_socket(Context &context, ssl_context &ssl) :
	base_type(new asio_ssl_socket_type(asio_basic_tcp_socket<Exec>(context), ssl), [this]
	{
		error_code error;
		auto sock = reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
		sock->shutdown(error);
//		native_object().close(error);
		delete sock;
	})
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_ssl_tcp_socket<Exec>::basic_ssl_tcp_socket(asio_basic_tcp_socket<Exec0> &&sock, ssl_context &ssl) :
	base_type(new asio_ssl_socket_type(std::move(sock), ssl), [this]
	{
		error_code error;
		auto sock = reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
		sock->shutdown(error);
//		native_object().close(error);
		delete sock;
	})
{

}

template <concept_execution Exec>
basic_ssl_tcp_socket<Exec>::basic_ssl_tcp_socket(const executor_type &exec, ssl_context &ssl) :
	base_type(new asio_ssl_socket_type(asio_basic_tcp_socket<Exec>(exec), ssl), [this]
	{
		error_code error;
		auto sock = reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
		sock->shutdown(error);
//		native_object().close(error);
		delete sock;
	})
{

}

template <concept_execution Exec>
basic_ssl_tcp_socket<Exec>::basic_ssl_tcp_socket(ssl_context &ssl) :
	base_type(new asio_ssl_socket_type(asio_basic_tcp_socket<Exec>(execution::io_context()), ssl), [this]
	{
		error_code error;
		auto sock = reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
		sock->shutdown(error);
//		native_object().close(error);
		delete sock;
	})
{

}

template <concept_execution Exec>
typename basic_ssl_tcp_socket<Exec>::asio_socket_type &basic_ssl_tcp_socket<Exec>::native_object()
{
	return reinterpret_cast<asio_ssl_socket_type*>(this->m_handle)->next_layer();
}

template <concept_execution Exec>
awaitable<error_code> basic_ssl_tcp_socket<Exec>::do_connect
(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept
{
	auto error = co_await base_type::do_connect(ep, cnl_sig);
	if( error )
		co_return error;

	auto &sock = *reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
	if( cnl_sig )
	{
		co_await sock.async_handshake
				(asio_ssl_socket_type::client, asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		co_await sock.async_handshake
				(asio_ssl_socket_type::client, use_awaitable_e[error]);
	}
	if( this->m_connect_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_connect_cancel = false;
	}
	co_return error;
}

template <concept_execution Exec>
awaitable<size_t> basic_ssl_tcp_socket<Exec>::read_data
(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	auto &sock = *reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
	this->m_read_cancel = false;

	if( rc.var.index() == 0 )
	{
		auto delim = std::get<0>(rc.var);
		if( delim.empty() )
		{
			if( cnl_sig )
			{
				buf.size = co_await sock.async_read_some
						(asio::buffer(buf.data, buf.size),
						 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await sock.async_read_some
						(asio::buffer(buf.data, buf.size), use_awaitable_e[error]);
			}
		}
		else
		{
			std::string _buf;
			if( cnl_sig )
			{
				buf.size = co_await asio::async_read_until
						(sock, asio::dynamic_buffer(_buf, buf.size), delim,
						 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await asio::async_read_until
						(sock, asio::dynamic_buffer(_buf, buf.size), delim, use_awaitable_e[error]);
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
					(sock, asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var),
					 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
		}
		else
		{
			buf.size = co_await asio::async_read_until
					(sock, asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var), use_awaitable_e[error]);
		}
		if( buf.size > 0 )
			memcpy(buf.data, _buf.c_str(), buf.size);
	}
	if( this->m_read_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_read_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec>
awaitable<size_t> basic_ssl_tcp_socket<Exec>::write_data
(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	auto &sock = *reinterpret_cast<asio_ssl_socket_type*>(this->m_handle);
	this->m_write_cancel = false;

	if( cnl_sig )
	{
		buf.size = co_await sock.async_write_some
				(asio::buffer(buf.data.data(), buf.size),
				 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		buf.size = co_await sock.async_write_some
				(asio::buffer(buf.data.data(), buf.size), use_awaitable_e[error]);
	}
	if( this->m_write_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		this->m_write_cancel = false;
	}
	co_return buf.size;
}

template <concept_execution Exec>
basic_ssl_tcp_socket<Exec>::basic_ssl_tcp_socket(auto *asio_sock, concept_callable auto &&del_sock) :
	base_type(asio_sock, std::forward<decltype(del_sock)>(del_sock))
{

}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H
