
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

#ifndef LIBGS_HTTP_SERVER_CONTEXT_H
#define LIBGS_HTTP_SERVER_CONTEXT_H

#include <libgs/http/server/response.h>
#include <libgs/http/server/session_set.h>

namespace libgs::http
{

template <concept_stream_requires Stream, concept_char_type CharT>
class basic_service_context
{
	LIBGS_DISABLE_COPY(basic_service_context)

public:
	using stream_t = Stream;
	using parser_t = basic_request_parser<CharT>;

	using request_t = basic_server_request<stream_t,CharT>;
	using response_t = basic_server_response<stream_t,CharT>;

	using session_t = basic_session<CharT>;
	using session_ptr = basic_session_ptr<CharT>;

public:
	basic_service_context(stream_t &&stream, parser_t &parser, session_set &sss);
	~basic_service_context();

	basic_service_context(basic_service_context &&other) noexcept;
	basic_service_context &operator=(basic_service_context &&other) noexcept;

	template<typename Stream0>
	basic_service_context(basic_service_context<Stream0,CharT> &&other) noexcept
		requires concept_constructible<Stream,Stream0&&>;

	template<typename Stream0>
	basic_service_context &operator=(basic_service_context<Stream0,CharT> &&other) noexcept
		requires concept_assignable<Stream,Stream0&&>;

public:
	const request_t &request() const noexcept;
	request_t &request() noexcept;

	const response_t &response() const noexcept;
	response_t &response() noexcept;

public:
	template <base_of_session<CharT> Session, typename...Args>
	std::shared_ptr<Session> session(Args&&...args)
		requires concept_constructible<Session, Args...>;

	template <typename...Args>
	session_ptr session(Args&&...args) noexcept
		requires concept_constructible<basic_session<CharT>, Args...>;

	template <base_of_session<CharT> Session>
	std::shared_ptr<Session> session() const;
	session_ptr session() const;

	template <base_of_session<CharT> Session>
	std::shared_ptr<Session> session_or();
	session_ptr session_or() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <concept_execution Exec>
using basic_tcp_service_context = basic_service_context<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using wbasic_tcp_service_context = basic_service_context<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_service_context = basic_tcp_service_context<asio::any_io_executor>;
using wtcp_service_context = wbasic_tcp_service_context<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/context.h>


#endif //LIBGS_HTTP_SERVER_CONTEXT_H

