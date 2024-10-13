
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

#ifndef LIBGS_HTTP_CXX_SOCKET_OPERATION_HELPER_H
#define LIBGS_HTTP_CXX_SOCKET_OPERATION_HELPER_H

#include <libgs/http/cxx/attributes.h>
#include <libgs/http/cxx/concepts.h>

namespace libgs::http
{

template <concepts::stream_requires Stream>
class socket_operation_helper;

template <concepts::stream_requires Stream>
class LIBGS_HTTP_TAPI socket_operation_helper_base
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper_base)

public:
	using socket_t = Stream;
	using executor_t = typename socket_t::executor_type;
	using endpoint_t = typename socket_t::endpoint_type;

public:
	socket_operation_helper_base(socket_t &socket);
	~socket_operation_helper_base();

public:
	[[nodiscard]] executor_t get_executor() noexcept;
	[[nodiscard]] const socket_t &socket() const noexcept;
	[[nodiscard]] socket_t &socket() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec>
class LIBGS_HTTP_TAPI socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>> :
	public socket_operation_helper_base<asio::basic_stream_socket<asio::ip::tcp,Exec>>
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper)

public:
	using base_t = socket_operation_helper_base<
		asio::basic_stream_socket<asio::ip::tcp,Exec>
	>;
	using base_t::base_t;

	using socket_t = typename base_t::socket_t;
	using executor_t = typename base_t::executor_t;
	using endpoint_t = typename base_t::endpoint_t;

public:
	template <asio::completion_token_for<void(error_code)> Token>
	[[nodiscard]] auto async_connect(endpoint_t ep, Token &&token);

	void connect(endpoint_t ep, error_code &error) noexcept;
	void connect(endpoint_t ep);

	void get_option(auto &option, error_code &error) noexcept;
	void get_option(auto &option);
	void close() noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() noexcept;
	[[nodiscard]] endpoint_t local_endpoint() noexcept;
	[[nodiscard]] bool is_open() noexcept;
};

#ifdef LIBGS_ENABLE_OPENSSL

template <core_concepts::execution Exec>
class LIBGS_HTTP_TAPI socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> :
	public socket_operation_helper_base<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>
{
	LIBGS_DISABLE_COPY_MOVE(socket_operation_helper)

public:
	using base_t = socket_operation_helper_base<
		asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>
	>;
	using base_t::base_t;

	using socket_t = typename base_t::socket_t;
	using executor_t = typename base_t::executor_t;
	using endpoint_t = typename base_t::endpoint_t;

public:
	template <asio::completion_token_for<void(error_code)> Token>
	[[nodiscard]] auto async_connect(endpoint_t endpoint, Token &&token);

	void connect(endpoint_t endpoint, error_code &error) noexcept;
	void connect(endpoint_t endpoint);

	void get_option(auto &option, error_code &error) noexcept;
	void get_option(auto &option);
	void close() noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint();
	[[nodiscard]] endpoint_t local_endpoint();
	[[nodiscard]] bool is_open() noexcept;
};

#endif //LIBGS_ENABLE_OPENSSL

} //namespace libgs::http
#include <libgs/http/cxx/detail/socket_operation_helper.h>


#endif //LIBGS_HTTP_CXX_SOCKET_OPERATION_HELPER_H
