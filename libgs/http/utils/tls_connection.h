
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_UTILS_TLS_CONNECTION_H
#define LIBGS_HTTP_UTILS_TLS_CONNECTION_H

#include <libgs/http/utils/connection.h>
#include <libgs/core/execution.h>

#if LIBGS_OPENSSL_SUPPORT

namespace libgs::http
{

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_tls_connection : public basic_connection<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_tls_connection)

public:
	using executor_t = Exec;
	using probe_state_t = connection_probe_state;

	using socket_t = asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,executor_t>>;
	using ptr_t = std::shared_ptr<basic_tls_connection>;

	explicit basic_tls_connection(socket_t &&socket);
	~basic_tls_connection() override;

public:
	sys_expected<> cancel() noexcept override;
	sys_expected<> close() noexcept override;

	sys_expected<> set_options(const tcp_socket_options &options) noexcept override;
	[[nodiscard]] sys_expected<tcp_socket_state> options() const noexcept override;

	[[nodiscard]] bool is_open() const noexcept override;
	[[nodiscard]] sys_expected<probe_state_t> probe() noexcept override;

	[[nodiscard]] endpoint remote_endpoint() const noexcept override;
	[[nodiscard]] endpoint local_endpoint() const noexcept override;

	[[nodiscard]] executor_t get_executor() noexcept override;

protected:
	[[nodiscard]] io_expected read_some(mutable_buffer buffer) noexcept override;
	[[nodiscard]] io_expected write_all(const const_buffer &buffer) noexcept override;
	[[nodiscard]] io_expected
	write_all(std::span<const const_buffer> buffers) noexcept override;

	[[nodiscard]] awaitable<io_expected> co_read_some(mutable_buffer buffer) noexcept override;
	[[nodiscard]] awaitable<io_expected> co_write_all(const const_buffer &buffer) noexcept override;
	[[nodiscard]] awaitable<io_expected>
	co_write_all(std::span<const const_buffer> buffers) noexcept override;

private:
	socket_t m_socket;
};

using tls_connection = basic_tls_connection<>;
using tls_connection_ptr = basic_tls_connection<>::ptr_t;

} //namespace libgs::http
#include <libgs/http/utils/detail/tls_connection.h>

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_UTILS_TLS_CONNECTION_H
