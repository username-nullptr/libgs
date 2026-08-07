
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_UTILS_SOCKET_OPERATION_HELPER_H
#define LIBGS_HTTP_NT_UTILS_SOCKET_OPERATION_HELPER_H

#include <libgs/http_nt/utils/opt_token.h>

namespace libgs::http_nt
{

template <concepts::stream Stream>
class socket_operation_helper;

template <concepts::stream Stream>
class LIBGS_HTTP_NT_TAPI socket_operation_helper_base
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper_base)

public:
	using socket_t = Stream;
	using executor_t = socket_t::executor_type;

public:
	explicit socket_operation_helper_base(socket_t &socket);
	~socket_operation_helper_base();

public:
	template <core_concepts::opt_token<error_code,size_t> Token = use_sync_t>
	[[nodiscard]] auto read(mutable_buffer buffer, Token &&token = {});

	template <core_concepts::opt_token<error_code,size_t> Token = use_sync_t>
	[[nodiscard]] auto write(const const_buffer &buffer, Token &&token = {});

	[[nodiscard]] io_expected try_read (
		mutable_buffer buffer
	) noexcept;

public:
	[[nodiscard]] executor_t get_executor() noexcept;
	[[nodiscard]] const socket_t &socket() const noexcept;
	[[nodiscard]] socket_t &socket() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::exec Exec>
class LIBGS_HTTP_NT_TAPI socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>> :
	public socket_operation_helper_base<asio::basic_stream_socket<asio::ip::tcp,Exec>>
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper)

public:
	using base_t = socket_operation_helper_base<
		asio::basic_stream_socket<asio::ip::tcp,Exec>
	>;
	using base_t::base_t;

	using socket_t = base_t::socket_t;
	using protocol_t = socket_t::protocol_type;

	using executor_t = base_t::executor_t;
	using endpoint_t = socket_t::endpoint_type;

	using dns_results = asio::ip::basic_resolver_results<protocol_t>;
	using dns_entry = dns_results::value_type;

public:
	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const core_concepts::text_p<char> auto &host, const value &service, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const endpoint_t &ep, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const dns_entry &ep, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const dns_results &eps, Token &&token = {});

public:
	void set_option(const auto &option, error_code &error) noexcept;
	void set_option(const auto &option);

	void get_option(auto &option, error_code &error) noexcept;
	void get_option(auto &option);

	void non_blocking(bool mode, error_code &error) noexcept;
	void non_blocking(bool mode) noexcept;

	[[nodiscard]] bool non_blocking() const noexcept;
	[[nodiscard]] bool message_peek() noexcept;

	void cancel() noexcept;
	void close() noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() const noexcept;
	[[nodiscard]] endpoint_t local_endpoint() const noexcept;
	[[nodiscard]] bool is_open() const noexcept;
};

#if LIBGS_OPENSSL_SUPPORT

template <core_concepts::exec Exec>
class LIBGS_HTTP_NT_TAPI socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> :
	public socket_operation_helper_base<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper)

public:
	using base_t = socket_operation_helper_base<
		asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>
	>;
	using base_t::base_t;

	using socket_t = base_t::socket_t;
	using protocol_t = socket_t::next_layer_type::protocol_type;

	using executor_t = base_t::executor_t;
	using endpoint_t = socket_t::next_layer_type::endpoint_type;

	using dns_results = asio::ip::basic_resolver_results<protocol_t>;
	using dns_entry = dns_results::value_type;

public:
	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const core_concepts::text_p<char> auto &host, const value &service, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const endpoint_t &ep, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const dns_entry &ep, Token &&token = {});

	template <core_concepts::opt_token<error_code> Token = use_sync_t>
	auto connect(const dns_results &eps, Token &&token = {});

public:
	void set_option(const auto &option, error_code &error) noexcept;
	void set_option(const auto &option);

	void get_option(auto &option, error_code &error) noexcept;
	void get_option(auto &option);

	void non_blocking(bool mode, error_code &error) noexcept;
	void non_blocking(bool mode) noexcept;

	[[nodiscard]] bool non_blocking() const noexcept;
	[[nodiscard]] bool message_peek() noexcept;

	void cancel() noexcept;
	void close() noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() const noexcept;
	[[nodiscard]] endpoint_t local_endpoint() const noexcept;
	[[nodiscard]] bool is_open() const noexcept;
};

#endif //LIBGS_OPENSSL_SUPPORT

} //namespace libgs::http_nt
#include <libgs/http_nt/utils/detail/socket_operation_helper.h>


#endif //LIBGS_HTTP_NT_UTILS_SOCKET_OPERATION_HELPER_H
