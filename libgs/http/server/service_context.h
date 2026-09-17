// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
	using executor_type = Exec;
	using executor_t = executor_type;

	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

	using request_t = basic_request<executor_t>;
	using response_t = basic_response<executor_t>;

	using session_t = http::session;
	using parser_t = request_t::parser_t;

public:
	basic_service_context (
		connection_ptr conn, session_manager &session_manager,
		std::filesystem::path resource_root = {}
	);
	basic_service_context (
		connection_ptr conn, parser_t &&parser, session_manager &session_manager,
		std::filesystem::path resource_root = {}
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
