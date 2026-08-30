
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

#ifndef LIBGS_HTTP_SERVER_SERVICE_CONTEXT_H
#define LIBGS_HTTP_SERVER_SERVICE_CONTEXT_H

#include <libgs/http/server/session_manager.h>
#include <libgs/http/server/response.h>
#include <libgs/http/server/request.h>

namespace libgs::http
{

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_service_context
{
	LIBGS_DISABLE_COPY_MOVE(basic_service_context)

public:
	using executor_t = Exec;
	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

	using request_t = basic_request<executor_t>;
	using response_t = basic_response<executor_t>;
	using session_t = http::session;
	using parser_t = request_t::parser_t;

public:
	basic_service_context (
		connection_ptr conn, session_manager &session_manager
	);
	basic_service_context (
		connection_ptr conn, parser_t &&parser,
		session_manager &session_manager
	);
	~basic_service_context();

public:
	[[nodiscard]] const request_t &request() const noexcept;
	[[nodiscard]] request_t &request() noexcept;

	[[nodiscard]] const response_t &response() const noexcept;
	[[nodiscard]] response_t &response() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;

	// Generic HTTP Upgrade ownership boundary. A future WebSocket module can use
	// this without coupling frame handling to the HTTP parser.
	[[nodiscard]] connection_ptr hand_over_connection() noexcept;
	[[nodiscard]] bool connection_handed_over() const noexcept;

public: // Fucking msvc !!!
	template <typename Session, typename...Args>
	[[nodiscard]] std::shared_ptr<Session> session(Args&&...args) requires
		core_concepts::base_of<Session,session_t> and core_concepts::constructible<Session, Args...>;

	template <typename...Args>
	[[nodiscard]] session_ptr session(Args&&...args)
		requires core_concepts::constructible<session_t, Args...>;

	template <typename Session>
	[[nodiscard]] std::shared_ptr<Session> session() const requires
		core_concepts::base_of<Session,session_t>;

	template <typename Session>
	[[nodiscard]] std::shared_ptr<Session> session_or() requires
		core_concepts::base_of<Session,session_t>;

	[[nodiscard]] session_ptr session() const;
	[[nodiscard]] session_ptr session_or() noexcept;

private:
	class impl;
	impl *m_impl;
};

using service_context = basic_service_context<>;

} //namespace libgs::http
#include <libgs/http/server/detail/service_context.h>


#endif //LIBGS_HTTP_SERVER_SERVICE_CONTEXT_H
