
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
#include <libgs/io/ip_types.h>

namespace libgs::io
{

template <concept_execution Exec = asio::any_io_executor>
class basic_socket : public basic_stream<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_socket)
	using base_type = basic_stream<Exec>;

public:
	using executor_type = base_type::executor_type;
	using address_vector = std::vector<ip_address>;
	using shutdown_type = asio::socket_base::shutdown_type;

	template <typename...Args>
	using cb_token = opt_token<callback_t<Args...>>;

public:
	using base_type::base_type;
	~basic_socket() override = default;

public:
	void connect(host_endpoint ep, cb_token<error_code> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(host_endpoint ep, opt_token<ua_redirect_error_t> tk) noexcept;
	void connect(host_endpoint ep, error_code &error) noexcept;

public:
	void connect(ip_endpoint ep, cb_token<error_code> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(ip_endpoint ep, opt_token<ua_redirect_error_t> tk) noexcept;
	virtual void connect(ip_endpoint ep, error_code &error) noexcept = 0;

public:
	[[nodiscard]] virtual ip_endpoint remote_endpoint(error_code &error) const noexcept = 0;
	[[nodiscard]] virtual ip_endpoint local_endpoint(error_code &error) const noexcept = 0;
	
	[[nodiscard]] ip_endpoint remote_endpoint() const noexcept;
	[[nodiscard]] ip_endpoint local_endpoint() const noexcept;

public:
	virtual void shutdown(error_code &error, shutdown_type what = shutdown_type::shutdown_both) noexcept = 0;
	void shutdown(shutdown_type what = shutdown_type::shutdown_both) noexcept;

	void close(error_code &error, bool shutdown) noexcept;
	void close(bool shutdown) noexcept;
	using base_type::close;

public:
	virtual void set_option(const socket_option &op, error_code &error) noexcept = 0;
	virtual void get_option(socket_option op, error_code &error) const noexcept = 0;

	void set_option(const socket_option &op) noexcept;
	void get_option(socket_option op) const noexcept;

public:
	void dns(std::string domain, cb_token<address_vector,error_code> tk) noexcept;
	void dns(std::wstring domain, cb_token<address_vector,error_code> tk) noexcept;

	void dns(std::string domain, cb_token<address_vector> tk) noexcept;
	void dns(std::wstring domain, cb_token<address_vector> tk) noexcept;

	[[nodiscard]] awaitable<address_vector> dns(std::string domain, opt_token<ua_redirect_error_t> tk) noexcept;
	[[nodiscard]] awaitable<address_vector> dns(std::wstring domain, opt_token<ua_redirect_error_t> tk) noexcept;

	[[nodiscard]] awaitable<address_vector> dns(std::string domain, opt_token<use_awaitable_t&> tk) noexcept;
	[[nodiscard]] awaitable<address_vector> dns(std::wstring domain, opt_token<use_awaitable_t&> tk) noexcept;

	virtual address_vector dns(std::string domain, error_code &error) noexcept = 0;
	address_vector dns(std::wstring domain, error_code &error) noexcept;

	address_vector dns(std::string domain) noexcept;
	address_vector dns(std::wstring domain) noexcept;

public:
	size_t read_buffer_size() const noexcept override;
	size_t write_buffer_size() const noexcept override;

protected:
	[[nodiscard]] virtual awaitable<error_code> do_connect(const ip_endpoint &ep) noexcept = 0;
	[[nodiscard]] virtual awaitable<address_vector> do_dns(const std::string &domain, error_code &error) noexcept = 0;
};

using socket = basic_socket<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_socket_ptr = std::shared_ptr<basic_socket<Exec>>;

using socket_ptr = basic_socket_ptr<>;

} //namespace libgs::io
#include <libgs/io/detail/socket.h>


#endif //LIBGS_IO_SOCKET_H
