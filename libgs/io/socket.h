
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
class LIBGS_IO_TAPI basic_socket : public basic_stream<crtp_derived<Derived,basic_socket<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_socket)
	using base_type = basic_stream<crtp_derived<Derived,basic_socket>,Exec>;

public:
	using executor_type = base_type::executor_type;
	using derived_type = crtp_derived_t<Derived, basic_socket>;

	using address_vector = std::vector<ip_address>;
	using shutdown_type = asio::socket_base::shutdown_type;

public:
	using base_type::base_type;
	~basic_socket() override = 0;

	basic_socket(basic_socket &&other) noexcept = default;
	basic_socket &operator=(basic_socket &&other) noexcept = default;

public:
	derived_type &connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(host_endpoint ep, opt_token<error_code&> tk = {});

	derived_type &connect(ip_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept;
	[[nodiscard]] awaitable<void> connect(ip_endpoint ep, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] ip_endpoint remote_endpoint() const requires requires
	{ static_cast<ip_endpoint(derived_type::*)(error_code&)>(derived_type::remote_endpoint); };

	[[nodiscard]] ip_endpoint local_endpoint() const requires requires
	{ static_cast<ip_endpoint(derived_type::*)(error_code&)>(derived_type::remote_endpoint); };

public:
	derived_type &set_option(const auto &op);
	derived_type &get_option(auto &op) const;

public:
	derived_type &dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<address_vector> dns(string_wrapper domain, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] size_t read_buffer_size() const noexcept;
	[[nodiscard]] size_t write_buffer_size() const noexcept;

/*** Derived class implementation required:
 *
 *  derived_type &set_option(const auto &op, error_code &error) noexcept;
 *  derived_type &get_option(auto &op, error_code &error) const noexcept;
 *
 *  [[nodiscard]] awaitable<error_code> _connect
 *  (const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept;
 *
 *  [[nodiscard]] awaitable<address_vector> _dns
 *  (const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept;
**/
};

} //namespace libgs::io
#include <libgs/io/detail/socket.h>


#endif //LIBGS_IO_SOCKET_H
