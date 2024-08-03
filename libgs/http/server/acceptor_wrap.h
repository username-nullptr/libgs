
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

#ifndef LIBGS_HTTP_SERVER_ACCEPTOR_WRAP_H
#define LIBGS_HTTP_SERVER_ACCEPTOR_WRAP_H

#include <libgs/http/global.h>
#include <libgs/core/coroutine.h>

namespace libgs::http
{

namespace detail
{

template <concept_execution Exec>
class LIBGS_HTTP_VAPI acceptor_wrap
{
	LIBGS_DISABLE_COPY(acceptor_wrap)

public:
	using executor_t = Exec;
	using acceptor_t = asio::basic_socket_acceptor<asio::ip::tcp,executor_t>;

public:
	acceptor_wrap(acceptor_t &&acceptor);
	~acceptor_wrap() = default;

	acceptor_wrap(acceptor_wrap &&other) noexcept = default;
	acceptor_wrap &operator=(acceptor_wrap &&other) noexcept = default;

	template <concept_execution Exec0>
	acceptor_wrap(acceptor_wrap<Exec0> &&other) noexcept;

	template <concept_execution Exec0>
	acceptor_wrap &operator=(acceptor_wrap<Exec0> &&other) noexcept;

public:
	const acceptor_t &acceptor() const;
	acceptor_t &acceptor();

protected:
	acceptor_t m_acceptor;
};

} //namespace detail

template <typename Stream>
class basic_acceptor_wrap;

template <concept_execution Exec>
class LIBGS_HTTP_TAPI basic_acceptor_wrap<asio::basic_stream_socket<asio::ip::tcp,Exec>> :
	public detail::acceptor_wrap<Exec>
{
	LIBGS_DISABLE_COPY(basic_acceptor_wrap)

public:
	using base_t = detail::acceptor_wrap<Exec>;
	using executor_t = typename base_t::executor_t;
	using acceptor_t = typename base_t::acceptor_t;

	template <typename Exec0>
	using basic_socket_t = asio::basic_stream_socket<asio::ip::tcp,Exec0>;
	using socket_t = basic_socket_t<executor_t>;

public:
	basic_acceptor_wrap(acceptor_t &&acceptor);
	~basic_acceptor_wrap() = default;

	basic_acceptor_wrap(basic_acceptor_wrap &&other) noexcept = default;
	basic_acceptor_wrap &operator=(basic_acceptor_wrap &&other) noexcept = default;

	template <concept_execution Exec0>
	basic_acceptor_wrap(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept;

	template <concept_execution Exec0>
	basic_acceptor_wrap &operator=(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept;

public:
	[[nodiscard]] awaitable<socket_t> accept(concept_execution auto &service_exec);
};

#ifdef LIBGS_ENABLE_OPENSSL

template <concept_execution Exec>
class LIBGS_HTTP_TAPI basic_acceptor_wrap<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> :
	public detail::acceptor_wrap<Exec>
{
	LIBGS_DISABLE_COPY(basic_acceptor_wrap)

public:
	using base_t = detail::acceptor_wrap<Exec>;
	using executor_t = typename base_t::executor_t;
	using acceptor_t = typename base_t::acceptor_t;

	template <typename Exec0>
	using basic_socket_t = asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec0>>;
	using socket_t = basic_socket_t<executor_t>;

public:
	basic_acceptor_wrap(acceptor_t &&acceptor, asio::ssl::context &ssl);
	~basic_acceptor_wrap() = default;

	basic_acceptor_wrap(basic_acceptor_wrap &&other) noexcept;
	basic_acceptor_wrap &operator=(basic_acceptor_wrap &&other) noexcept;

	template <concept_execution Exec0>
	basic_acceptor_wrap(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept;

	template <concept_execution Exec0>
	basic_acceptor_wrap &operator=(basic_acceptor_wrap<basic_socket_t<Exec0>> &&other) noexcept;

public:
	[[nodiscard]] awaitable<socket_t> accept(concept_execution auto &service_exec);

protected:
	asio::ssl::context *m_ssl;
};

#endif //LIBGS_ENABLE_OPENSSL

using acceptor_wrap = basic_acceptor_wrap<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/acceptor_wrap.h>


#endif //LIBGS_HTTP_SERVER_ACCEPTOR_WRAP_H
