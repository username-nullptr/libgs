
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

#ifndef LIBGS_IO_UDP_SOCKET_H
#define LIBGS_IO_UDP_SOCKET_H

#include <libgs/io/socket.h>

namespace libgs
{

template <concept_execution Exec>
using asio_basic_udp_socket = asio::basic_stream_socket<asio::ip::udp, Exec>;

using asio_udp_socket = asio_basic_udp_socket<asio::any_io_executor>;
using udp_handle_type = asio::ip::udp::socket::native_handle_type;

namespace io
{

template <concept_execution Exec, typename Derived = void>
class LIBGS_IO_TAPI basic_udp_socket : public basic_socket<crtp_derived_t<Derived,basic_udp_socket<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_udp_socket)

public:
	using derived_t = crtp_derived_t<Derived,basic_udp_socket>;
	using base_t = basic_socket<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using address_vector = base_t::address_vector;

	using native_t = asio_basic_udp_socket<executor_t>;
	using resolver_t = asio::ip::udp::resolver;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_udp_socket(Context &context = execution::io_context());

	template <concept_execution Exec0>
	basic_udp_socket(asio_basic_udp_socket<Exec0> &&native);

	explicit basic_udp_socket(const executor_t &exec);
	~basic_udp_socket() override;

public:
	template <concept_execution Exec0>
	basic_udp_socket(basic_udp_socket<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_udp_socket &operator=(basic_udp_socket<Exec0,Derived> &&other) noexcept;

	template <concept_execution Exec0>
	basic_udp_socket &operator=(asio_basic_udp_socket<Exec0> &&native) noexcept;

public:
	// TODO ...
#if 0
	[[nodiscard]] awaitable<size_t> write()
	{
		asio::ip::udp::socket sss;
		sss.async_send_to();
	}

	derived_t &read(host_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read(host_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept;

	derived_t &read(host_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read(host_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t>> tk) noexcept;
	derived_t &read(host_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<error_code>> tk) noexcept;
	derived_t &read(host_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read(host_endpoint ep, buffer<void*> buf, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(host_endpoint ep, buffer<std::string&> buf, opt_token<error_code&> tk = {});

	derived_t &read(ip_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read(ip_endpoint ep, buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept;

	derived_t &read(ip_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read(ip_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<size_t>> tk) noexcept;
	derived_t &read(ip_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<error_code>> tk) noexcept;
	derived_t &read(ip_endpoint ep, buffer<std::string&> buf, opt_token<callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read(ip_endpoint ep, buffer<void*> buf, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(ip_endpoint ep, buffer<std::string&> buf, opt_token<error_code&> tk = {});




	derived_t &write(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_t &write(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept;
	[[nodiscard]] awaitable<size_t> write(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

#endif

public:
	[[nodiscard]] const native_t &native() const noexcept;
	[[nodiscard]] native_t &native() noexcept;

	[[nodiscard]] const resolver_t &resolver() const noexcept;
	[[nodiscard]] resolver_t &resolver() noexcept;

protected:
	[[nodiscard]] awaitable<size_t> _read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept;

	[[nodiscard]] awaitable<size_t> _write_data
	(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept;

	void _cancel() noexcept;
	resolver_t m_resolver;

private:
	friend class basic_stream<basic_udp_socket,executor_t>;
	static void _delete_native(void *native);
};

using udp_socket = basic_udp_socket<asio::any_io_executor>;

}} //namespace libgs::io
#include <libgs/io/detail/udp_socket.h>


#endif //LIBGS_IO_UDP_SOCKET_H
