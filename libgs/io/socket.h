
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

template <typename Derived, concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI basic_socket : public basic_stream<crtp_derived_t<Derived,basic_socket<Derived,Exec>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_socket)

public:
	using derived_t = crtp_derived_t<Derived,basic_socket>;
	using base_t = basic_stream<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using address_vector = std::vector<ip_address>;

public:
	using basic_stream<derived_t,Exec>::basic_stream;
	~basic_socket() override = 0;

	template <concept_execution Exec0>
	basic_socket(basic_socket<Derived,Exec0> &&other) noexcept;

	template <concept_execution Exec0>
	basic_socket &operator=(basic_socket<Derived,Exec0> &&other) noexcept;

public:
	derived_t &connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(host_endpoint ep, opt_token<error_code&> tk = {});

	derived_t &connect(ip_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(ip_endpoint ep, opt_token<error_code&> tk = {});

	derived_t &bind(ip_endpoint ep, no_time_token tk = {});

public:
	derived_t &dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<address_vector> dns(string_wrapper domain, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] ip_endpoint remote_endpoint(no_time_token tk = {}) const;
	[[nodiscard]] ip_endpoint local_endpoint(no_time_token tk = {}) const;

public:
	derived_t &set_option(const socket_option &op, no_time_token tk = {});
	derived_t &get_option(socket_option op, no_time_token tk = {});
	const derived_t &get_option(socket_option op, no_time_token tk = {}) const;

public:
	[[nodiscard]] size_t read_buffer_size() const noexcept;
	[[nodiscard]] size_t write_buffer_size() const noexcept;

/*** Derived class implementation required:
 *
 *  [[nodiscard]] const resolver_type &resolver() const noexcept;
 *  [[nodiscard]] resolver_type &resolver() noexcept;
**/
protected:
	[[nodiscard]] awaitable<error_code> _connect
	(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept;

	[[nodiscard]] awaitable<address_vector> _dns
	(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept;

	bool m_connect_cancel = false;
	bool m_dns_cancel = false;
};

} //namespace libgs::io
#include <libgs/io/detail/socket.h>


#endif //LIBGS_IO_SOCKET_H
