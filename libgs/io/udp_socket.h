
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

// TODO ...

#ifndef LIBGS_IO_UDP_SOCKET_H
#define LIBGS_IO_UDP_SOCKET_H

#include <libgs/io/socket.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_udp_socket = asio::basic_stream_socket<asio::ip::udp, Exec>;

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_udp_socket_ptr = std::shared_ptr<asio_basic_udp_socket<Exec>>;

using asio_udp_socket = asio_basic_udp_socket<>;
using asio_udp_socket_ptr = asio_basic_udp_socket_ptr<>;

using udp_handle_type = asio::ip::udp::socket::native_handle_type;

namespace io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_CORE_TAPI basic_udp_socket : public basic_socket<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_udp_socket)
	using base_type = basic_socket<Exec>;

public:
	using executor_type = base_type::executor_type;
	using address_vector = base_type::address_vector;
	using shutdown_type = base_type::shutdown_type;

	using asio_socket = asio_basic_udp_socket<Exec>;
	using asio_socket_ptr = asio_basic_udp_socket_ptr<Exec>;
	using resolver = asio::ip::udp::resolver;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_udp_socket(Context &context = execution::io_context());

	template <concept_execution Exec0>
	basic_udp_socket(asio_basic_udp_socket<Exec0> &&sock);

	explicit basic_udp_socket(const executor_type &exec);
	~basic_udp_socket() override;

public: // TODO ...
	// recv_from
	// send_to

public: // TODO ...
	// join_multicast_group
	// leave_multicast_group

public:
	void connect(ip_endpoint ep, opt_token<error_code&> tk) override;
	address_vector dns(string_wrapper domain, opt_token<error_code&> tk) override;

public:
	size_t read(buffer<void*> buf, read_token<error_code&> tk) override;
	size_t write(buffer<const void*> buf, opt_token<error_code&> tk) override;

public:
	[[nodiscard]] ip_endpoint local_endpoint(error_code &error) const noexcept override;

public:
	void shutdown(error_code &error, shutdown_type what) noexcept override;
	void close(error_code &error) noexcept override;

public:
	[[nodiscard]] bool is_open() const noexcept override;
	void cancel() noexcept override;

public:
	void set_option(const socket_option &op, error_code &error) noexcept override;
	void get_option(socket_option op, error_code &error) const noexcept override;

public:
	[[nodiscard]] const asio_socket &native_object() const;
	[[nodiscard]] asio_socket &native_object();
	[[nodiscard]] udp_handle_type native_handle() const;

public:
	using base_type::connect;
	using base_type::dns;
	using base_type::read;
	using base_type::write;
	using base_type::local_endpoint;
	using base_type::shutdown;
	using base_type::close;
	using base_type::set_option;

protected:
	[[nodiscard]] awaitable<error_code> do_connect(const ip_endpoint &ep) noexcept override;
	[[nodiscard]] awaitable<address_vector> do_dns(const std::string &domain, error_code &error) noexcept override;

protected:
	[[nodiscard]] awaitable<size_t> read_data(void *buf, size_t size, error_code &error) noexcept override;
	[[nodiscard]] awaitable<size_t> write_data(const void *buf, size_t size, error_code &error) noexcept override;

protected:
	explicit basic_udp_socket(auto *asio_sock, concept_callable auto &&del_sock);

protected:
	void *m_sock;
	std::function<void()> m_del_sock {
		[this]{ delete reinterpret_cast<asio_socket*>(m_sock); }
	};
	resolver m_resolver;
	std::function<void()> m_del_cb {nullptr};

	std::atomic_bool m_write_cancel {false};
	std::atomic_bool m_read_cancel {false};

	std::atomic_bool m_connect_cancel {false};
	std::atomic_bool m_dns_cancel {false};
};

using udp_socket = basic_udp_socket<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_udp_socket_ptr = std::shared_ptr<basic_udp_socket<Exec>>;

using udp_socket_ptr = basic_udp_socket_ptr<>;

template <concept_execution Exec, typename...Args>
basic_udp_socket_ptr<Exec> make_basic_udp_socket(Args&&...args);

template <typename...Args>
udp_socket_ptr make_udp_socket(Args&&...args);

}} //namespace libgs::io
// #include <libgs/io/detail/udp_socket.h>

namespace libgs::io
{

template <concept_execution Exec>
template <concept_execution_context Context>
basic_udp_socket<Exec>::basic_udp_socket(Context &context) :
	base_type(context.get_executor()),
	m_sock(std::make_shared<asio_socket>(context)),
	m_resolver(context)
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_udp_socket<Exec>::basic_udp_socket(asio_basic_udp_socket<Exec0> &&sock) :
	base_type(sock.get_executor()),
	m_sock(std::make_shared<asio_socket>(std::move(sock))),
	m_resolver(m_sock->get_executor())
{

}

template <concept_execution Exec>
basic_udp_socket<Exec>::basic_udp_socket(const executor_type &exec) :
	base_type(exec),
	m_sock(std::make_shared<asio_socket>(exec)),
	m_resolver(m_sock->get_executor())
{

}

template <concept_execution Exec>
basic_udp_socket<Exec>::~basic_udp_socket()
{
	error_code error;
	m_sock->shutdown(shutdown_type::shutdown_both, error);
	m_sock->close(error);
}

template <concept_execution Exec>
ip_endpoint basic_udp_socket<Exec>::local_endpoint(error_code &error) const noexcept
{
	auto ep = m_sock->local_endpoint(error);
	return {ep.address(), ep.port()};
}

template <concept_execution Exec>
void basic_udp_socket<Exec>::shutdown(error_code &error, shutdown_type what) noexcept
{
	m_sock->shutdown(what, error);
}

template <concept_execution Exec>
void basic_udp_socket<Exec>::close(error_code &error) noexcept
{
	m_sock->close(error);
}

template <concept_execution Exec>
bool basic_udp_socket<Exec>::is_open() const noexcept
{
	return m_sock->is_open();
}

template <concept_execution Exec>
void basic_udp_socket<Exec>::cancel() noexcept
{
	m_write_cancel = true;
	m_read_cancel = true;

	m_connect_cancel = true;
	m_dns_cancel = true;

	m_resolver.cancel();
	m_sock->cancel();
}

template <concept_execution Exec>
void basic_udp_socket<Exec>::set_option(const socket_option &op, error_code &error) noexcept
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

	else error = std::make_error_code(errc::invalid_argument);
}

template <concept_execution Exec>
void basic_udp_socket<Exec>::get_option(socket_option op, error_code &error) const noexcept
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

	else error = std::make_error_code(errc::invalid_argument);
}

template <concept_execution Exec>
typename basic_udp_socket<Exec>::asio_socket &basic_udp_socket<Exec>::native_object() const
{
	return *m_sock;
}

template <concept_execution Exec>
udp_handle_type basic_udp_socket<Exec>::native_handle() const
{
	return m_sock->native_handle();
}

template <concept_execution Exec>
awaitable<error_code> basic_udp_socket<Exec>::do_connect(const endpoint &ep) noexcept 
{
	error_code error;
	m_connect_cancel = false;
	co_await m_sock->async_connect({ep.addr, ep.port}, use_awaitable_e[error]);

	if( m_connect_cancel )
	{
		error = std::make_error_code(errc::operation_aborted);
		m_connect_cancel = false;
	}
	co_return error;
}

template <concept_execution Exec>
awaitable<typename basic_udp_socket<Exec>::address_vector> basic_udp_socket<Exec>::do_dns
(const std::string &domain, error_code &error) noexcept 
{
	address_vector vector;

	m_dns_cancel = false;
	auto results = co_await m_resolver.async_resolve(domain, "0", use_awaitable_e[error]);

	if( m_dns_cancel )
	{
		error = std::make_error_code(errc::operation_aborted);
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
awaitable<size_t> basic_udp_socket<Exec>::read_data(void *buf, size_t size, error_code &error) noexcept 
{
	m_read_cancel = false;
	size = co_await m_sock->async_read_some(asio::buffer(buf, size), use_awaitable_e[error]);

	if( m_read_cancel )
	{
		error = std::make_error_code(errc::operation_aborted);
		m_read_cancel = false;
	}
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_udp_socket<Exec>::write_data(const void *buf, size_t size, error_code &error) noexcept 
{
	m_write_cancel = false;
	size = co_await m_sock->async_write_some(asio::buffer(buf, size), use_awaitable_e[error]);

	if( m_write_cancel )
	{
		error = std::make_error_code(errc::operation_aborted);
		m_write_cancel = false;
	}
	co_return size;
}

template <concept_execution Exec, typename...Args>
basic_udp_socket_ptr<Exec> make_basic_udp_socket(Args&&...args)
{
	return std::make_shared<basic_udp_socket<Exec>>(std::forward<Args>(args)...);
}

template <typename...Args>
udp_socket_ptr make_udp_socket(Args&&...args)
{
	return std::make_shared<udp_socket>(std::forward<Args>(args)...);
}

} //namespace libgs::io


#endif //LIBGS_IO_UDP_SOCKET_H
