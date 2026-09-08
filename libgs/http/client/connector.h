// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_CLIENT_CONNECTOR_H
#define LIBGS_HTTP_CLIENT_CONNECTOR_H

#include <libgs/http/utils/connection.h>
#include <libgs/http/cxx/concepts.h>

namespace libgs::http
{

enum class security_mode {
	plain, tls, // ... ...
};

struct connect_target
{
	std::string host {};
	uint16_t port = 0;
	security_mode security {};

	friend bool operator== (
		const connect_target&, const connect_target&
	) = default;
};

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_connector
{
	LIBGS_DISABLE_COPY_MOVE(basic_connector)

public:
	using executor_t = Exec;
	using ptr_t = std::shared_ptr<basic_connector>;

	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

public:
	explicit basic_connector (
		core_concepts::match_sched<executor_t> auto &&exec
	);
#if LIBGS_OPENSSL_SUPPORT
	basic_connector (
		core_concepts::match_sched<executor_t> auto &&exec,
		asio::ssl::context &tls_context
	);
#endif //LIBGS_OPENSSL_SUPPORT
	virtual ~basic_connector();

	template <typename Token = use_sync_t>
	auto connect(const connect_target &target, Token &&token = {}) noexcept
		requires concepts::dis_detach_opt_token<Token,error_code,connection_ptr>;

	[[nodiscard]] executor_t get_executor() const noexcept;

protected:
	// The default implementation connects directly. Proxy support is provided by
	// deriving a configured connector; callers that do not configure one never
	// need to model proxy routes in their connect_target.
	[[nodiscard]] virtual sys_expected<connection_ptr>
	do_connect(const connect_target &target) noexcept;

	[[nodiscard]] virtual awaitable<sys_expected<connection_ptr>>
	co_do_connect(const connect_target &target) noexcept;

private:
	class impl;
	impl *m_impl;
};

using connector = basic_connector<>;

} //namespace libgs::http
#include <libgs/http/client/detail/connector.h>


#endif //LIBGS_HTTP_CLIENT_CONNECTOR_H
