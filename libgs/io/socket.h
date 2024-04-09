
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

#ifndef LIBGS_IO_SOCKET_H
#define LIBGS_IO_SOCKET_H

#include <libgs/io/stream.h>
#include <libgs/io/types/ip_types.h>

namespace libgs::io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_CORE_TAPI basic_socket : public basic_stream<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_socket)
	using base_type = basic_stream<Exec>;

public:
	using executor_type = base_type::executor_type;
	using address_vector = std::vector<ip_address>;
	using shutdown_type = asio::socket_base::shutdown_type;

public:
	using base_type::base_type;
	~basic_socket() override = default;

public:
	void connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(host_endpoint ep, opt_token<error_code&> tk = {});

	void connect(ip_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(ip_endpoint ep, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] virtual ip_endpoint remote_endpoint(error_code &error) const noexcept = 0;
	[[nodiscard]] ip_endpoint remote_endpoint() const;

	[[nodiscard]] virtual ip_endpoint local_endpoint(error_code &error) const noexcept = 0;
	[[nodiscard]] ip_endpoint local_endpoint() const;

public:
	virtual void shutdown(error_code &error, shutdown_type what) noexcept = 0;
	void shutdown(shutdown_type what = shutdown_type::shutdown_both);
	void shutdown(error_code &error) noexcept;

	void close(error_code &error, bool shutdown) noexcept;
	void close(bool shutdown);
	using base_type::close;

public:
	virtual void set_option(const socket_option &op, error_code &error) noexcept = 0;
	virtual void get_option(socket_option op, error_code &error) const noexcept = 0;

	void set_option(const socket_option &op);
	void get_option(socket_option op) const;

public:
	void dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<address_vector> dns(string_wrapper domain, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] size_t read_buffer_size() const noexcept override;
	[[nodiscard]] size_t write_buffer_size() const noexcept override;

protected:
	[[nodiscard]] virtual awaitable<error_code> do_connect
	(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept = 0;

	[[nodiscard]] virtual awaitable<address_vector> do_dns
	(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept = 0;
};

using socket = basic_socket<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_socket_ptr = std::shared_ptr<basic_socket<Exec>>;

using socket_ptr = basic_socket_ptr<>;

} //namespace libgs::io
#include <libgs/io/detail/socket.h>


#endif //LIBGS_IO_SOCKET_H
