
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

#ifndef LIBGS_IO_TCP_SERVER_H
#define LIBGS_IO_TCP_SERVER_H

#include <libgs/io/tcp_socket.h>
#include <set>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor = asio::basic_socket_acceptor<asio::ip::tcp, Exec>;

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor_ptr = std::shared_ptr<asio_basic_tcp_acceptor<Exec>>;

using asio_tcp_acceptor = asio_basic_tcp_acceptor<>;
using asio_tcp_acceptor_ptr = asio_basic_tcp_acceptor_ptr<>;

namespace io
{

template <concept_execution Exec = asio::any_io_executor>
class basic_tcp_server : public device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_tcp_server)
	using base_type = device_base<Exec>;
	class client_socket_type;

public:
	using executor_type = base_type::executor_type;
	using asio_acceptor = asio_basic_tcp_acceptor<Exec>;
	using asio_acceptor_ptr = asio_basic_tcp_acceptor_ptr<Exec>;

public:
	explicit basic_tcp_server(size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution_context Context>
	explicit basic_tcp_server(Context &context, size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution Exec0>
	explicit basic_tcp_server(asio_acceptor &&acceptor, size_t tcount = std::thread::hardware_concurrency() << 1);

	explicit basic_tcp_server(const executor_type &exec, size_t tcount = std::thread::hardware_concurrency() << 1);
	~basic_tcp_server() override;

public:
	void bind(ip_endpoint ep, error_code &error, size_t max = asio::socket_base::max_listen_connections) noexcept;
	void bind(ip_endpoint ep, size_t max = asio::socket_base::max_listen_connections);

	awaitable<void> co_cancel() noexcept;
	void cancel() noexcept override;

public:
	void async_accept(opt_cb_token<tcp_socket_ptr,error_code> tk) noexcept;
	[[nodiscard]] awaitable<tcp_socket_ptr> co_accept(opt_token<error_code&> tk = {});
	tcp_socket_ptr accept(opt_token<error_code&> tk = {});

public:
	void set_option(const auto &op, error_code &error);
	void set_option(const auto &op);

	void get_option(auto &op, error_code &error);
	void get_option(auto &op);

public:
	awaitable<void> co_wait() noexcept;
	void wait() noexcept;

protected:
	explicit basic_tcp_server(auto *asio_acceptor, concept_callable auto &&del_acceptor);

private:
	const asio_acceptor &acceptor() const;
	asio_acceptor &acceptor();

protected:
	void *m_acceptor;
	std::function<void()> m_del_acceptor {
		[this]{ delete reinterpret_cast<asio_acceptor*>(m_acceptor); }
	};
	asio::thread_pool m_pool;
	std::set<tcp_socket*> m_sock_set;
};

using tcp_server = basic_tcp_server<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_tcp_server_ptr = std::shared_ptr<basic_tcp_server<Exec>>;

using tcp_server_ptr = basic_tcp_server_ptr<>;

template <concept_execution Exec, typename...Args>
basic_tcp_server_ptr<Exec> make_basic_tcp_server(Args&&...args);

template <typename...Args>
tcp_server_ptr make_tcp_server(Args&&...args);

} //namespace io

} //namespace libgs
// #include <libgs/io/detail/tco_server.h>

namespace libgs::io
{

template <concept_execution Exec>
class basic_tcp_server<Exec>::client_socket_type : public tcp_socket
{
	LIBGS_DISABLE_COPY_MOVE(client_socket_type)
	using base_type = tcp_socket;

public:
	explicit client_socket_type(asio_tcp_socket &&sock) :
		base_type(std::move(sock)) {}

	~client_socket_type() override 
	{
		assert(m_del_cb);
		m_del_cb();
	}

	void set_delete_callback(callback_t<> del_cb) 
	{
		assert(del_cb);
		m_del_cb = std::move(del_cb);
	}

private:
	callback_t<> m_del_cb;
};

template <concept_execution Exec>
basic_tcp_server<Exec>::basic_tcp_server(size_t tcount) :
	base_type(io_context().get_executor()),
	m_acceptor(new asio_acceptor(io_context())),
	m_pool(tcount)
{

}

template <concept_execution Exec>
template <concept_execution_context Context>
basic_tcp_server<Exec>::basic_tcp_server(Context &context, size_t tcount) :
	base_type(context.get_executor()),
	m_acceptor(new asio_acceptor(context)),
	m_pool(tcount)
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_tcp_server<Exec>::basic_tcp_server(asio_acceptor &&acceptor, size_t tcount) :
	base_type(acceptor.get_executor()),
	m_acceptor(new asio_acceptor(std::move(acceptor))),
	m_pool(tcount)
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::basic_tcp_server(const executor_type &exec, size_t tcount) :
	base_type(exec),
	m_acceptor(new asio_acceptor(exec)),
	m_pool(tcount)
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::~basic_tcp_server()
{
	cancel();
	if( m_acceptor and m_del_acceptor )
		m_del_acceptor();
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::bind(ip_endpoint ep, error_code &error, size_t max) noexcept
{
	auto &apr = acceptor();
	if( not apr.is_open() )
	{
		if( ep.addr.is_v4() )
			apr.open(asio::ip::tcp::v4(), error);
		else
			apr.open(asio::ip::tcp::v6(), error);
		if( error )
			return ;
	}
	apr.set_option(asio_tcp_acceptor::reuse_address(true), error);
	if( error )
		return;

	apr.bind({std::move(ep.addr), ep.port}, error);
	if( not error )
		apr.listen(max, error);
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::bind(ip_endpoint ep, size_t max)
{
	error_code error;
	bind(std::move(ep), error, max);

	if( error )
		throw system_error(error, "libgs::io::tcp_server::bind");
}

template <concept_execution Exec>
awaitable<void> basic_tcp_server<Exec>::co_cancel() noexcept
{
	return co_thread([this]{cancel();});
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::cancel() noexcept
{
	error_code error;
	acceptor().cancel(error);
	acceptor().close(error);

	auto sock_set = std::move(m_sock_set);
	for(auto &sock : sock_set)
		sock->close(error);
	wait();
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::async_accept(opt_cb_token<tcp_socket_ptr,error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		auto size = co_await co_accept({tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<tcp_socket_ptr> basic_tcp_server<Exec>::co_accept(opt_token<error_code&> tk)
{
	error_code error;
	auto socket = co_await acceptor().async_accept(m_pool, use_awaitable_e[error]);

	std::shared_ptr<client_socket_type> sock_ptr;
	if( error )
	{
		if( tk.error == nullptr )
			throw system_error(error, "libgs::io::stream::co_read");

		*tk.error = std::move(error);
		co_return sock_ptr;
	}
	sock_ptr = std::make_shared<client_socket_type>(std::move(socket));	
	auto _sock_ptr = sock_ptr.get();
	m_sock_set.emplace(_sock_ptr);

	auto valid = this->m_valid;
	sock_ptr->set_delete_callback([this, valid = std::move(valid), _sock_ptr]
	{
		if( not valid )
			return ;

		asio::post(this->m_exec, [this, valid = std::move(valid), _sock_ptr]
		{
			if( valid )
				m_sock_set.erase(_sock_ptr);
		});
	});
	co_return sock_ptr;
}

template <concept_execution Exec>
tcp_socket_ptr basic_tcp_server<Exec>::accept(opt_token<error_code&> tk)
{
	error_code error;
	auto socket = acceptor().accept(m_pool, error);

	std::shared_ptr<client_socket_type> sock_ptr;
	if( error )
	{
		if( tk.error == nullptr )
			throw system_error(error, "libgs::io::stream::co_read");

		*tk.error = std::move(error);
		return sock_ptr;
	}
	sock_ptr = std::make_shared<client_socket_type>(std::move(socket));	
	auto _sock_ptr = sock_ptr.get();
	m_sock_set.emplace(_sock_ptr);

	auto valid = this->m_valid;
	sock_ptr->set_delete_callback([this, valid = std::move(valid), _sock_ptr]
	{
		if( not valid )
			return ;

		asio::post(this->m_exec, [this, valid = std::move(valid), _sock_ptr]
		{
			if( valid )
				m_sock_set.erase(_sock_ptr);
		});
	});
	return sock_ptr;
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::set_option(const auto &op, error_code &error)
{
	acceptor().set_option(op, error);
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::set_option(const auto &op)
{
	error_code error;
	set_option(op, error);

	if( error )
		throw system_error(error, "libgs::io::tcp_server::set_option");
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::get_option(auto &op, error_code &error)
{
	acceptor().get_option(op, error);
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::get_option(auto &op)
{
	error_code error;
	get_option(op, error);

	if( error )
		throw system_error(error, "libgs::io::tcp_server::get_option");
}

template <concept_execution Exec>
awaitable<void> basic_tcp_server<Exec>::co_wait() noexcept
{
	return co_thread([this]{wait();});
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::wait() noexcept
{
	m_pool.wait();
}

template <concept_execution Exec>
basic_tcp_server<Exec>::basic_tcp_server(auto *asio_acceptor, concept_callable auto &&del_acceptor) :
	base_type(asio_acceptor->get_executor()),
	m_acceptor(asio_acceptor),
	m_del_acceptor(std::forward<std::decay_t<decltype(del_acceptor)>>(del_acceptor))
{
	assert(asio_acceptor);
	assert(m_del_acceptor);
}

template <concept_execution Exec>
const typename basic_tcp_server<Exec>::asio_acceptor &basic_tcp_server<Exec>::acceptor() const
{
	return *reinterpret_cast<const asio_acceptor*>(m_acceptor);
}

template <concept_execution Exec>
typename basic_tcp_server<Exec>::asio_acceptor &basic_tcp_server<Exec>::acceptor()
{
	return *reinterpret_cast<asio_acceptor*>(m_acceptor);
}

} //namespace libgs::io


#endif //LIBGS_IO_TCP_SERVER_H
