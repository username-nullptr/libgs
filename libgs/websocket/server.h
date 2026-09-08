// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_SERVER_H
#define LIBGS_WEBSOCKET_SERVER_H

#include <libgs/websocket/stream.h>
#include <libgs/http/server.h>

namespace libgs::websocket
{

// Snapshot retained after the HTTP service context is destroyed.
struct request_info
{
	http::method_enum method = http::method::get;
	http::version_enum version = http::version::v11;
	std::string target {};
	std::string path {};
	http::headers request_headers {};
	http::parameters query_parameters {};
	http::parameters path_arguments {};
	http::endpoint remote_endpoint {};
	http::endpoint local_endpoint {};
};

// A complete HTTP response for a policy-level opening-handshake rejection.
// The body is owned so it remains valid until an asynchronous response finishes.
struct upgrade_rejection
{
	http::status_enum status = http::status::forbidden;
	http::headers headers {};
	std::string body {};
};

// Empty accepts the request; a value rejects it with the contained response.
using upgrade_validation_result = std::optional<upgrade_rejection>;

// Per-connection server handshake and stream policy.
struct upgrade_options
{
	using request_validator_t = std::function<upgrade_validation_result(
		const request_info&
	)>;
	using origin_validator_t = std::function<upgrade_validation_result(
		std::optional<std::string_view>
	)>;
	using subprotocol_selector_t = std::function<std::optional<std::string>(
		std::span<const std::string>
	)>;
	using extension_selector_t = std::function<std::vector<extension>(
		std::span<const extension>
	)>;

	stream_config stream {};
	// Covers validation and writing the final opening-handshake response after
	// this request is selected for upgrade. It is separate from the owned
	// server's pending-handshake queue timeout. The deadline can preempt
	// asynchronous I/O; synchronous I/O checks it between blocking calls.
	std::chrono::milliseconds handshake_timeout {30000};
	std::vector<std::string> supported_subprotocols {};
	// Wire-level offers reserved for post-baseline extension support. A future
	// implementation may enable one only when a matching frame codec capability
	// is installed; the baseline rejects non-empty local configuration.
	std::vector<extension> supported_extensions {};
	// Additional 101 response headers. Protocol-owned upgrade headers cannot be
	// replaced through this collection.
	http::headers response_headers {};
	bool require_subprotocol = false;
	request_validator_t request_validator {};
	origin_validator_t origin_validator {};
	subprotocol_selector_t subprotocol_selector {};
	// Handshake policy only. Returning an extension can never enable it without
	// a matching installed frame codec capability.
	extension_selector_t extension_selector {};
};

struct upgrade_result
{
	std::string subprotocol {};
	// Always empty in the baseline implementation.
	std::vector<extension> extensions {};
};

struct server_config
{
	upgrade_options default_upgrade {};
	size_t max_pending_handshakes = 64;
	// Non-positive prevents a request from waiting in the pending-handshake
	// queue. A request paired immediately with an accept operation does not wait.
	std::chrono::milliseconds pending_handshake_timeout {30000};
};

// Move-only result of a completed server opening handshake.
template <core_concepts::exec Exec = asio::any_io_executor>
struct basic_accept_result
{
	using executor_t = Exec;
	using stream_t = basic_stream<executor_t>;

	explicit basic_accept_result (
		core_concepts::match_sched<executor_t> auto &&exec
	) :
		stream(std::forward<decltype(exec)>(exec)) {}

	explicit basic_accept_result(stream_t value) :
		stream(std::move(value)) {}

	basic_accept_result (
		stream_t value, request_info request_value,
		upgrade_result handshake_value = {}
	) :
		stream(std::move(value)),
		request(std::move(request_value)),
		handshake(std::move(handshake_value)) {}

	stream_t stream;
	request_info request {};
	upgrade_result handshake {};
};

using accept_result = basic_accept_result<>;

template <core_concepts::exec Exec>
[[nodiscard]] bool is_upgrade_request (
	const http::basic_request<Exec> &request
) noexcept;

// Upgrade adapter for mixed HTTP/WebSocket servers.
template <core_concepts::exec Exec, typename Token = use_sync_t>
[[nodiscard]] auto upgrade (
	http::basic_service_context<Exec> &context,
	Token &&token = {}
) requires
	concepts::dis_detach_opt_token<
		Token,error_code,basic_accept_result<Exec>
	>;

template <core_concepts::exec Exec, typename Token = use_sync_t>
[[nodiscard]] auto upgrade (
	http::basic_service_context<Exec> &context,
	upgrade_options options,
	Token &&token = {}
) requires
	concepts::dis_detach_opt_token<
		Token,error_code,basic_accept_result<Exec>
	>;

// WebSocket acceptor backed by an owned HTTP server.
template <http::concepts::any_exec_stream Stream = asio::ip::tcp::socket>
class LIBGS_WEBSOCKET_TAPI basic_server
{
	LIBGS_DISABLE_COPY_MOVE(basic_server)

public:
	using socket_t = Stream;
	using executor_t = socket_t::executor_type;
	using config_t = server_config;
	using stream_t = basic_stream<executor_t>;
	using upgrade_options_t = upgrade_options;
	using upgrade_result_t = upgrade_result;
	using request_info_t = request_info;
	using accept_result_t = basic_accept_result<executor_t>;

	using http_server_t = http::basic_server<socket_t>;
	using http_config_t = http_server_t::config_t;
	using acceptor_wrap_t = http_server_t::acceptor_wrap_t;
	using endpoint_wrapper_t = http_server_t::endpoint_wrapper_t;
	using path_opt_token_t = http_server_t::path_opt_token_t;
	using context_t = http_server_t::context_t;
	using server_error_handler_t = http_server_t::server_error_handler_t;
	using service_error_handler_t = http_server_t::service_error_handler_t;

	template <typename Token>
	static constexpr bool accept_token_v =
		concepts::dis_detach_opt_token<Token,error_code,accept_result_t>;

	template <typename Func>
	static constexpr bool connection_handler_v = requires(Func &&func, accept_result_t value) {
		{ std::forward<Func>(func)(std::move(value)) } -> core_concepts::awaitable;
		requires std::same_as <
			libgs::awaitable_ret_t<decltype(
				std::forward<Func>(func)(std::move(value))
			)>, void
		>;
	};

public:
	basic_server (
		acceptor_wrap_t &&wrap,
		core_concepts::sched auto &&service_exec,
		config_t config = {}
	);
	explicit basic_server(acceptor_wrap_t &&wrap, config_t config = {});
	~basic_server();

public:
	basic_server &bind(endpoint_wrapper_t endpoint);
	basic_server &bind(endpoint_wrapper_t endpoint, error_code &error) noexcept;

	basic_server &start(size_t max = asio::socket_base::max_listen_connections);
	basic_server &start(size_t max, error_code &error) noexcept;
	basic_server &start(error_code &error) noexcept;

	basic_server &start (
		core_concepts::sched auto &&service_exec,
		size_t max = asio::socket_base::max_listen_connections
	);
	basic_server &start (
		core_concepts::sched auto &&service_exec,
		size_t max, error_code &error
	) noexcept;
	basic_server &start (
		core_concepts::sched auto &&service_exec,
		error_code &error
	) noexcept;

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto accept(Token &&token = {})
		requires accept_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto accept(upgrade_options_t options, Token &&token = {})
		requires accept_token_v<Token>;

	[[nodiscard]] size_t pending_accept_count() const noexcept;
	[[nodiscard]] size_t pending_handshake_count() const noexcept;

public:
	template <typename Func>
	basic_server &on_connection (
		const path_opt_token_t &path_rules,
		Func &&func,
		std::optional<upgrade_options_t> options = std::nullopt
	) requires connection_handler_v<Func>;

	template <typename Func>
	basic_server &on_default (
		Func &&func,
		std::optional<upgrade_options_t> options = std::nullopt
	) requires connection_handler_v<Func>;

	template <core_concepts::text_p<char> Text>
	basic_server &unbound_connection(const Text &path_rule = "");

	basic_server &on_server_error(server_error_handler_t func);
	basic_server &on_service_error(service_error_handler_t func);
	basic_server &unbound_server_error();
	basic_server &unbound_service_error();

public:
	// Replaces the snapshot used by future accept operations and future pending
	// handshake admissions. Existing operations, queued requests and registered
	// handlers retain the snapshots captured when they were created.
	basic_server &set_config(const config_t &config);
	[[nodiscard]] config_t config() const;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_server &cancel() noexcept;
	basic_server &stop() noexcept;

	[[nodiscard]] const http_server_t &http_server() const noexcept;
	[[nodiscard]] http_server_t &http_server() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <core_concepts::exec Exec = asio::any_io_executor>
using basic_tcp_server = basic_server<
	asio::basic_stream_socket<asio::ip::tcp,Exec>
>;

using tcp_server = basic_tcp_server<>;
using server = tcp_server;

} //namespace libgs::websocket
#include <libgs/websocket/detail/server.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs::websocket
{

template <typename Protocol = asio::ip::tcp,
	core_concepts::exec Exec = asio::any_io_executor>
using basic_ssl_server = basic_server<
	asio::ssl::stream<asio::basic_stream_socket<Protocol,Exec>>
>;

template <core_concepts::exec Exec = asio::any_io_executor>
using basic_tls_server = basic_ssl_server<asio::ip::tcp,Exec>;

using tls_server = basic_tls_server<>;
using ssl_tcp_server = tls_server;
using ssl_server = tls_server;

} //namespace libgs::websocket
#endif //LIBGS_OPENSSL_SUPPORT


#endif //LIBGS_WEBSOCKET_SERVER_H
