
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
	void bind(ip_endpoint &ep, error_code &error) noexcept;
	void bind(ip_endpoint &ep);

	awaitable<void> cancel(use_awaitable_t &token) noexcept;
	void cancel() noexcept override;

public:
	void accept(opt_cb_token<tcp_socket_ptr,error_code> token) noexcept;
	void accept(opt_cb_token<tcp_socket_ptr> token);

	[[nodiscard]] awaitable<tcp_socket_ptr> accept(opt_token<ua_redirect_error_t> token) noexcept;
	[[nodiscard]] awaitable<tcp_socket_ptr> accept(opt_token<use_awaitable_t&> token);

	tcp_socket_ptr accept(error_code &error) noexcept;
	tcp_socket_ptr accept();

public:
	awaitable<void> wait(use_awaitable_t &token) noexcept;
	void wait() noexcept;

	awaitable<void> stop(use_awaitable_t &token) noexcept;
	void stop() noexcept;

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
	explicit client_socket_type(asio_tcp_socket &&sock, callback_t<> del_cb) :
		base_type(std::move(sock)), m_del_cb(std::move(del_cb)) {}

	~client_socket_type() override 
	{
		assert(m_del_cb);
		m_del_cb();
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


// TODO ...

template <concept_execution Exec>
awaitable<void> basic_tcp_server<Exec>::cancel(use_awaitable_t&) noexcept
{
	return co_thread([this]{cancel();});
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::cancel() noexcept
{
	error_code error;
	acceptor().close(error);

	wait();
	m_pool.stop();

	// TODO ...
}

template <concept_execution Exec>
awaitable<void> basic_tcp_server<Exec>::wait(use_awaitable_t&) noexcept
{
	return co_thread([this]{wait();});
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::wait() noexcept
{
	m_pool.wait();
}

template <concept_execution Exec>
awaitable<void> basic_tcp_server<Exec>::stop(use_awaitable_t&) noexcept
{
	return co_thread([this]{stop();});
}

template <concept_execution Exec>
void basic_tcp_server<Exec>::stop() noexcept
{
	cancel();
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
