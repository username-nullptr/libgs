
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_CLIENT_CONTEXT_H
#define LIBGS_HTTP_CLIENT_CONTEXT_H

#include <libgs/http/client/request.h>
#include <libgs/http/client/reply.h>

namespace libgs::http
{

template <protocol::method_enum Method,
		  concepts::connection Connection,
		  protocol::version_enum Version = protocol::version::v11>
class LIBGS_HTTP_TAPI basic_request_context
{
	LIBGS_DISABLE_COPY(basic_request_context)

public:
	using connection_t = Connection;
	using executor_t = connection_t::executor_t;

	static constexpr auto method_v = Method;
	static constexpr auto version_v = Version;

	using request_t = basic_client_request<method_v,connection_t,version_v>;
	using reply_t = basic_reply<connection_t>;

public:
	explicit basic_request_context(request_t &&request);
	~basic_request_context();

	basic_request_context(basic_request_context &&other) noexcept;
	basic_request_context &operator=(basic_request_context &&other) noexcept;

public:
	[[nodiscard]] const request_t &request() const noexcept;
	[[nodiscard]] request_t &request() noexcept;

	[[nodiscard]] const reply_t &reply() const noexcept;
	[[nodiscard]] reply_t &reply() noexcept;

public:
	template <core_concepts::tf_opt_token<error_code,protocol::status_enum> Token = use_sync_t>
	auto wait_reply(Token &&token = {}) noexcept;

	[[nodiscard]] bool responded() const noexcept;
	[[nodiscard]] executor_t get_executor() const noexcept;
	basic_request_context &cancel() noexcept;

private:
	class impl;
	impl *m_impl = nullptr;
};

template <protocol::method_enum Method, protocol::version_enum Version = protocol::version::v11>
using request_context = basic_request_context<Method, connection, Version>;

} //namespace libgs::http
#include <libgs/http/client/detail/context.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http
{

template <protocol::method_enum Method, protocol::version_enum Version = protocol::version::v11>
using ssl_request_context = basic_request_context<Method, ssl_connection, Version>;

} //namespace http

namespace https
{

template <http::protocol::method_enum Method, http::protocol::version_enum Version = http::protocol::version::v11>
using request_context = http::ssl_request_context<Method, Version>;

}} //namespace libgs::https

#endif //LIBGS_ENABLE_OPENSS
#endif //LIBGS_HTTP_CLIENT_CONTEXT_H