
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

template <concept_execution Exec>
using asio_basic_tcp_socket = asio::basic_stream_socket<asio::ip::tcp, Exec>;

using asio_tcp_socket = asio_basic_tcp_socket<asio::any_io_executor>;
using tcp_handle_type = asio::ip::tcp::socket::native_handle_type;

namespace io
{

template <concept_execution Exec, typename Derived = void>
class LIBGS_IO_TAPI basic_tcp_socket : public basic_socket<crtp_derived_t<Derived,basic_tcp_socket<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_tcp_socket)

public:
	using derived_t = crtp_derived_t<Derived,basic_tcp_socket>;
	using base_t = basic_socket<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using address_vector = base_t::address_vector;

	using native_t = asio_basic_tcp_socket<executor_t>;
	using resolver_t = asio::ip::tcp::resolver;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_tcp_socket(Context &context = execution::io_context());

	template <concept_execution Exec0>
	basic_tcp_socket(asio_basic_tcp_socket<Exec0> &&native);

	explicit basic_tcp_socket(const executor_t &exec);
	~basic_tcp_socket() override;

public:
	template <concept_execution Exec0>
	basic_tcp_socket(basic_tcp_socket<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_tcp_socket &operator=(basic_tcp_socket<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_tcp_socket &operator=(asio_basic_tcp_socket<Exec0> &&native) noexcept;

public:
	[[nodiscard]] ip_endpoint remote_endpoint(no_time_token tk = {}) const;
	[[nodiscard]] ip_endpoint local_endpoint(no_time_token tk = {}) const;

public:
	derived_t &set_option(const socket_option &op, no_time_token tk = {});
	derived_t &get_option(socket_option op, no_time_token tk = {});
	const derived_t &get_option(socket_option op, no_time_token tk = {}) const;

	[[nodiscard]] size_t read_buffer_size() const noexcept;
	[[nodiscard]] size_t write_buffer_size() const noexcept;

public:
	[[nodiscard]] const native_t &native() const noexcept;
	[[nodiscard]] native_t &native() noexcept;

	[[nodiscard]] const resolver_t &resolver() const noexcept;
	[[nodiscard]] resolver_t &resolver() noexcept;

protected:
	void _cancel() noexcept;
	resolver_t m_resolver;

private:
	friend class basic_stream<basic_tcp_socket,executor_t>;
	static void _delete_native(void *native);
};

using tcp_socket = basic_tcp_socket<asio::any_io_executor>;

}} //namespace libgs::io
#include <libgs/io/detail/tcp_socket.h>


#endif //LIBGS_IO_TCP_SOCKET_H
