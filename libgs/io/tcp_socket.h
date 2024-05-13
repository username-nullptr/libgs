
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

#ifndef LIBGS_IO_TCP_SOCKET_H
#define LIBGS_IO_TCP_SOCKET_H

#include <libgs/io/socket.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_socket = asio::basic_stream_socket<asio::ip::tcp, Exec>;

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_socket_ptr = std::shared_ptr<asio_basic_tcp_socket<Exec>>;

using asio_tcp_socket = asio_basic_tcp_socket<>;
using asio_tcp_socket_ptr = asio_basic_tcp_socket_ptr<>;

using tcp_handle_type = asio::ip::tcp::socket::native_handle_type;

namespace io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI basic_tcp_socket : public basic_socket<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_tcp_socket)
	using base_type = basic_socket<Exec>;

public:
	using executor_type = base_type::executor_type;
	using address_vector = base_type::address_vector;
	using shutdown_type = base_type::shutdown_type;

	using asio_socket_type = asio_basic_tcp_socket<Exec>;
	using asio_socket_ptr = asio_basic_tcp_socket_ptr<Exec>;
	using resolver = asio::ip::tcp::resolver;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_tcp_socket(Context &context = execution::io_context());

	template <concept_execution Exec0>
	basic_tcp_socket(asio_basic_tcp_socket<Exec0> &&sock);

	explicit basic_tcp_socket(const executor_type &exec);
	~basic_tcp_socket() override = default;

public:
	[[nodiscard]] ip_endpoint remote_endpoint(error_code &error) const noexcept override;
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
	[[nodiscard]] virtual asio_socket_type &native_object();
	[[nodiscard]] const asio_socket_type &native_object() const;
	[[nodiscard]] tcp_handle_type native_handle() const;

public:
	using base_type::remote_endpoint;
	using base_type::local_endpoint;
	using base_type::shutdown;
	using base_type::close;
	using base_type::set_option;

protected:
	[[nodiscard]] awaitable<error_code> do_connect
	(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept override;

	[[nodiscard]] awaitable<address_vector> do_dns
	(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept override;

	[[nodiscard]] awaitable<size_t> read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept override;

	[[nodiscard]] awaitable<size_t> write_data
	(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept override;

protected:
	explicit basic_tcp_socket(auto *asio_sock, concept_callable auto &&del_sock);

	resolver m_resolver;
	std::function<void()> m_del_cb {nullptr};

	std::atomic_bool m_write_cancel {false};
	std::atomic_bool m_read_cancel {false};

	std::atomic_bool m_connect_cancel {false};
	std::atomic_bool m_dns_cancel {false};
};

using tcp_socket = basic_tcp_socket<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_tcp_socket_ptr = std::shared_ptr<basic_tcp_socket<Exec>>;

using tcp_socket_ptr = basic_tcp_socket_ptr<>;

template <concept_execution Exec, typename...Args>
basic_tcp_socket_ptr<Exec> make_basic_tcp_socket(Args&&...args);

template <typename...Args>
tcp_socket_ptr make_tcp_socket(Args&&...args);

}} //namespace libgs::io
#include <libgs/io/detail/tcp_socket.h>


#endif //LIBGS_IO_TCP_SOCKET_H
