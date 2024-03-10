
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
	asio::thread_pool &pool();

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

}} //namespace libgs::io
#include <libgs/io/detail/tcp_server.h>


#endif //LIBGS_IO_TCP_SERVER_H
