
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

#ifndef LIBGS_HTTP_NT_SERVER_SERVICE_CONTEXT_H
#define LIBGS_HTTP_NT_SERVER_SERVICE_CONTEXT_H

#include <libgs/http_nt/server/session_manager.h>
#include <libgs/http_nt/server/response.h>
#include <libgs/http_nt/server/request.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_service_context
{
	LIBGS_DISABLE_COPY_MOVE(basic_service_context)

public:
	using connection_t = Connection;
	using connection_ptr = std::shared_ptr<connection_t>;
	using executor_t = connection_t::executor_t;

	using request_t = basic_request<connection_t>;
	using response_t = basic_response<connection_t>;
	using session_t = http_nt::session;

public:
	basic_service_context (
		connection_ptr connection, session_manager &session_manager
	);
	~basic_service_context();

public:
	[[nodiscard]] const request_t &request() const noexcept;
	[[nodiscard]] request_t &request() noexcept;

	[[nodiscard]] const response_t &response() const noexcept;
	[[nodiscard]] response_t &response() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;

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

using service_context = basic_service_context<connection>;

} //namespace libgs::http_nt
#include <libgs/http_nt/server/detail/service_context.h>


#endif //LIBGS_HTTP_NT_SERVER_SERVICE_CONTEXT_H
