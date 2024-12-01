
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

#ifndef LIBGS_HTTP_CXX_SOCKET_SESSION_H
#define LIBGS_HTTP_CXX_SOCKET_SESSION_H

#include <libgs/http/cxx/socket_operation_helper.h>

namespace libgs::http
{

template <concepts::stream Stream>
class LIBGS_HTTP_TAPI basic_socket_session
{
	LIBGS_DISABLE_COPY(basic_socket_session)

public:
	using socket_t = Stream;
    using opt_helper_t = socket_operation_helper<socket_t>;

	using executor_t = typename opt_helper_t::executor_t;
	using endpoint_t = typename opt_helper_t::endpoint_t;

public:
  	template <typename Func>
	basic_socket_session(socket_t &&socket, Func &&destructor)
		requires core_concepts::callable<Func,socket_t&&>;

	basic_socket_session(socket_t &&socket);
	basic_socket_session();
    ~basic_socket_session();

	basic_socket_session(basic_socket_session &&other) noexcept;
	basic_socket_session &operator=(basic_socket_session &&other) noexcept;
	basic_socket_session &operator=(socket_t &&socket) noexcept;

public:
 	[[nodiscard]] const socket_t &socket() const noexcept;
 	[[nodiscard]] socket_t &socket() noexcept;

 	[[nodiscard]] const opt_helper_t &opt_helper() const noexcept;
 	[[nodiscard]] opt_helper_t &opt_helper() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec = asio::any_io_executor>
using basic_tcp_socket_session = basic_socket_session<asio::basic_stream_socket<asio::ip::tcp,Exec>>;

using tcp_socket_session = basic_tcp_socket_session<asio::any_io_executor>;
using socket_session = tcp_socket_session;

} //namespace libgs::http
#include <libgs/http/cxx/detail/socket_session.h>


#endif //LIBGS_HTTP_CXX_SOCKET_SESSION_H
