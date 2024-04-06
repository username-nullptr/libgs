
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

#ifndef LIBGS_IO_DETAIL_TCP_SERVER_H
#define LIBGS_IO_DETAIL_TCP_SERVER_H

namespace libgs::io
{

template <concept_execution Exec>
class LIBGS_CORE_TAPI basic_tcp_server<Exec>::client_socket_type : public tcp_socket
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
basic_tcp_server<Exec>::start_token::start_token(size_t max, error_code &error) :
	max(max), error(&error)
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::start_token::start_token(error_code &error) :
	error(&error)
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::basic_tcp_server(size_t tcount) :
	base_type(execution::io_context().get_executor()),
	m_acceptor(new asio_acceptor(execution::io_context())),
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
void basic_tcp_server<Exec>::bind(ip_endpoint ep, opt_token<error_code&> tk)
{
	error_code error;
	auto &apr = acceptor();

	if( tk.error )
		*tk.error = error;

	auto lambda_error = [&]()
	{
		if( tk.error )
		{
			*tk.error = error;
			return ;
		}
		else
			throw system_error(error, "libgs::io::tcp_server::bind");
	};
	if( not apr.is_open())
	{
		if( ep.addr.is_v4())
			apr.open(asio::ip::tcp::v4(), error);
		else
			apr.open(asio::ip::tcp::v6(), error);

		if( error )
			return lambda_error();
	}
	apr.set_option(asio_tcp_acceptor::reuse_address(true), error);
	if( error )
		return lambda_error();

	apr.bind({std::move(ep.addr), ep.port}, error);
	if( error )
		return lambda_error();
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::start(start_token tk)
{
	error_code error;
	if( tk.error )
		*tk.error = error;

	acceptor().listen(static_cast<int>(tk.max), error);
	if( not error )
		return ;

	if( tk.error )
	{
		*tk.error = error;
		return ;
	}
	else
		throw system_error(error, "libgs::io::tcp_server::start");
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::async_accept(opt_token<callback_t<tcp_socket_ptr,error_code>> tk) noexcept
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

		*tk.error = error;
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

		*tk.error = error;
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
void basic_tcp_server<Exec>::set_non_block(bool flag, error_code &error) noexcept
{
	acceptor().non_blocking(flag, error);
}

template <concept_execution Exec>
bool basic_tcp_server<Exec>::is_non_block() const noexcept
{
	return acceptor().non_blocking();
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
asio::thread_pool &basic_tcp_server<Exec>::pool()
{
	return m_pool;
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


#endif //LIBGS_IO_DETAIL_TCP_SERVER_H
