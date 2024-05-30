
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

#ifndef LIBGS_IO_SSL_TCP_SOCKET_H
#define LIBGS_IO_SSL_TCP_SOCKET_H

#ifdef LIBGS_ENABLE_OPENSSL

#include <libgs/io/tcp_socket.h>
#include <libgs/io/ssl_stream.h>

namespace libgs::io
{

template <concept_execution Exec, typename Derived = void>
class LIBGS_IO_TAPI basic_ssl_tcp_socket :
	public basic_socket<crtp_derived_t<Derived,basic_ssl_tcp_socket<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_ssl_tcp_socket)

public:
	using derived_t = crtp_derived_t<Derived,basic_ssl_tcp_socket>;
	using base_t = basic_socket<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using address_vector = base_t::address_vector;

	using next_layer_t = basic_tcp_socket<executor_t>;
	using native_t = ssl_stream<next_layer_t>;
	using resolver_t = typename next_layer_t::resolver;

public:
//	template <concept_execution_context Context = asio::io_context>
//	explicit basic_ssl_tcp_socket(Context &context = execution::io_context());

//	explicit basic_ssl_tcp_socket(const executor_t &exec);
//	basic_ssl_tcp_socket(native_t &&native);


	template <concept_execution_context Context>
	basic_ssl_tcp_socket(Context &context, ssl::context &ssl);

	basic_ssl_tcp_socket(const executor_t &exec, ssl::context &ssl);
	explicit basic_ssl_tcp_socket(ssl::context &ssl);

	basic_ssl_tcp_socket(auto next_layer, ssl::context &ssl) requires requires
	{
		native_t(std::move(next_layer.native()), ssl);
		next_layer_t(std::move(next_layer));
	};
	basic_ssl_tcp_socket(native_t &&native);
	~basic_ssl_tcp_socket() override = default;

public:
	basic_ssl_tcp_socket(basic_ssl_tcp_socket &&other) noexcept;
	basic_ssl_tcp_socket &operator=(basic_ssl_tcp_socket &&other) noexcept;
	basic_ssl_tcp_socket &operator=(native_t &&native) noexcept;

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
	[[nodiscard]] awaitable<error_code> _close(cancellation_signal *cnl_sig);
	void _cancel() noexcept;

private:
	friend class basic_stream<basic_ssl_tcp_socket,executor_t>;
	static void _delete_native(void *native);
};

using ssl_tcp_socket = basic_ssl_tcp_socket<asio::any_io_executor>;

} //namespace libgs::io
#include <libgs/io/detail/ssl_tcp_socket.h>

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_IO_SSL_TCP_SOCKET_H
