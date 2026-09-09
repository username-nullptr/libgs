// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_CLIENT_H
#define LIBGS_WEBSOCKET_CLIENT_H

#include <libgs/websocket/stream.h>
#include <libgs/http/client.h>

namespace libgs::websocket
{

struct client_config
{
	stream_config stream {};
	// Non-positive means the opening handshake is already timed out. The deadline
	// can preempt asynchronous I/O; synchronous I/O checks it between calls.
	std::chrono::milliseconds handshake_timeout {30000};
};

// Per-connection overrides for an opening handshake.
struct connect_request
{
	url endpoint {};
	http::request_arg request_options {};
	std::optional<stream_config> stream_options {};
	// Non-positive means the opening handshake is already timed out. The deadline
	// can preempt asynchronous I/O; synchronous I/O checks it between calls.
	std::optional<std::chrono::milliseconds> handshake_timeout {};
	std::vector<std::string> subprotocols {};
	// Wire-level offers reserved for post-baseline extension support. Enabling an
	// offer will additionally require an installed frame codec capability; the
	// baseline rejects a non-empty value before network I/O starts.
	std::vector<extension> extensions {};
	size_t max_redirects = 0;
	bool allow_insecure_redirects = false;

	explicit connect_request(url value) : endpoint(std::move(value)) {}

	explicit connect_request(core_concepts::string_p<char> auto &&value) :
		endpoint(std::forward<decltype(value)>(value)) {}
};

// Optional out-parameter for retaining the final HTTP opening response.
// After a successful 101 handshake the reply remains usable as response
// metadata, but its connection lease has been transferred to websocket::stream.
template <core_concepts::exec Exec = asio::any_io_executor>
struct basic_open_diagnostics
{
	using executor_t = Exec;
	using reply_t = http::basic_reply<executor_t>;
	using reply_ptr = std::shared_ptr<reply_t>;

	url endpoint {};
	reply_ptr reply {};
};

using open_diagnostics = basic_open_diagnostics<>;

// Opening adapter for mixed HTTP/WebSocket clients.
template <core_concepts::exec Exec, http::version_enum Version,
	typename Token = use_sync_t>
[[nodiscard]] auto open (
	http::basic_client<Exec,Version> &http_client,
	connect_request request,
	Token &&token = {}
) requires
	(Version == http::version::v11) and
	concepts::dis_detach_opt_token<
		Token,error_code,basic_stream<Exec>
	>;

template <core_concepts::exec Exec, http::version_enum Version,
	typename Token = use_sync_t>
[[nodiscard]] auto open (
	http::basic_client<Exec,Version> &http_client,
	connect_request request,
	basic_open_diagnostics<Exec> &diagnostics,
	Token &&token = {}
) requires
	(Version == http::version::v11) and
	concepts::dis_detach_opt_token<
		Token,error_code,basic_stream<Exec>
	>;

template <core_concepts::exec Exec, http::version_enum Version,
	typename Token = use_sync_t>
[[nodiscard]] auto open (
	http::basic_client<Exec,Version> &http_client,
	url endpoint,
	Token &&token = {}
) requires
	(Version == http::version::v11) and
	concepts::dis_detach_opt_token<
		Token,error_code,basic_stream<Exec>
	>;

template <core_concepts::exec Exec, http::version_enum Version,
	typename Token = use_sync_t>
[[nodiscard]] auto open (
	http::basic_client<Exec,Version> &http_client,
	url endpoint,
	basic_open_diagnostics<Exec> &diagnostics,
	Token &&token = {}
) requires
	(Version == http::version::v11) and
	concepts::dis_detach_opt_token<
		Token,error_code,basic_stream<Exec>
	>;

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_WEBSOCKET_TAPI basic_client
{
	LIBGS_DISABLE_COPY(basic_client)

public:
	using executor_t = Exec;
	using config_t = client_config;
	using connect_request_t = connect_request;
	using http_client_t = http::basic_client<executor_t>;
	using stream_t = basic_stream<executor_t>;
	using diagnostics_t = basic_open_diagnostics<executor_t>;

public:
	template <typename Token>
	static constexpr bool open_token_v =
		concepts::dis_detach_opt_token<Token,error_code,stream_t>;

public:
	basic_client() requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_client(config_t config) requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_client (
		core_concepts::match_sched<executor_t> auto &&exec,
		config_t config = {}
	);

	// Transfers ownership of an HTTP/1.1 client into this connector.
	explicit basic_client(http_client_t &&http_client, config_t config = {});

	basic_client(basic_client &&other) noexcept;
	basic_client &operator=(basic_client &&other) noexcept;
	~basic_client();

public:
	// Each successful call returns an independently owned stream.
	template <typename Token = use_sync_t>
	[[nodiscard]] auto open(connect_request_t request, Token &&token = {})
		requires open_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto open (
		connect_request_t request, diagnostics_t &diagnostics,
		Token &&token = {}
	) requires open_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto open(url endpoint, Token &&token = {})
		requires open_token_v<Token>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto open (
		url endpoint, diagnostics_t &diagnostics, Token &&token = {}
	) requires open_token_v<Token>;

	[[nodiscard]] std::shared_ptr<http::cookie_jar> cookie_store() noexcept;

	[[nodiscard]] const http_client_t &http_client() const noexcept;
	[[nodiscard]] http_client_t &http_client() noexcept;

	[[nodiscard]] size_t pending_open_count() const noexcept;
	[[nodiscard]] config_t config() const noexcept;
	[[nodiscard]] executor_t get_executor() const noexcept;

	// Does not affect streams already returned by open().
	basic_client &cancel() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using client = basic_client<>;

} //namespace libgs::websocket
#include <libgs/websocket/detail/client.h>


#endif //LIBGS_WEBSOCKET_CLIENT_H
