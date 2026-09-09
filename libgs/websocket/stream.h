// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_STREAM_H
#define LIBGS_WEBSOCKET_STREAM_H

#include <libgs/websocket/types.h>
#include <libgs/http/utils/connection.h>

namespace libgs::websocket { namespace detail
{

template <core_concepts::exec Exec>
class stream_impl;

} //namespace detail

// One upgraded WebSocket session and its transport.
template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_WEBSOCKET_TAPI basic_stream
{
	LIBGS_DISABLE_COPY(basic_stream)

public:
	using executor_t = Exec;
	using config_t = stream_config;
	using close_frame_t = close_frame;
	using close_info_t = close_info;
	using control_event_t = control_event;
	using adopt_options_t = adopt_options;

	using connection_t = http::basic_connection<executor_t>;
	using connection_ptr = std::shared_ptr<connection_t>;

	template <typename Buffer>
	using recv_buf = basic_message<Buffer>;

	using body_type = message_type;

public:
	basic_stream() requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_stream(config_t config) requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_stream (
		core_concepts::match_sched<executor_t> auto &&exec,
		config_t config = {}
	);

	basic_stream(basic_stream &&other) noexcept;
	basic_stream &operator=(basic_stream &&other) noexcept;
	~basic_stream();

public:
	template <typename Token, typename...Args>
	static constexpr bool task_token_v =
		concepts::dis_detach_opt_token<Token,error_code,Args...>;

	// Detached completion is observed through the corresponding wait operation.
	template <typename Token, typename...Args>
	static constexpr bool completion_token_v =
		core_concepts::tf_opt_token<Token,error_code,Args...>;

	template <typename T>
	static constexpr bool message_buffer_v =
		libgs::is_buffer_v<std::remove_cvref_t<T>> and
		not is_array_buffer_v<std::remove_cvref_t<T>>;

public:
	// Adopts a connection after a validated HTTP Upgrade.
	basic_stream &adopt(connection_ptr connection, adopt_options_t options);

	basic_stream &adopt (
		connection_ptr connection, adopt_options_t options,
		error_code &error
	) noexcept;

public:
	// Reads one complete data message. Once a cleanly closed stream reaches the
	// closed state, read completes with eof and a default result whose body is
	// empty. Cancelling an asynchronous read preserves any partially parsed frame
	// so a later read can resume it.
	template <typename Buffer = std::vector<std::byte>, typename Token = use_sync_t>
	[[nodiscard]] auto read(Token &&token = {}) requires
		message_buffer_v<Buffer> and
		task_token_v<Token,basic_message<std::remove_cvref_t<Buffer>>>;

public:
	// Concurrent writes are serialized. Successful size_t results count bytes
	// from the user payload only; WebSocket frame headers and masking never count.
	// Cancellation removes an operation while it is queued. Once its first frame
	// owns the transport write, cancellation is too late and may be ignored.
	template <message_type Type, typename Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {})
		requires completion_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto write(message_type type, const const_buffer &body, Token &&token = {})
		requires completion_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto write (
		message_type type, std::span<const const_buffer> body,
		Token &&token = {}
	) requires completion_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto write_text(std::string_view text, Token &&token = {})
		requires completion_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto write_binary(const const_buffer &body, Token &&token = {})
		requires completion_token_v<Token,size_t>;

	// Waits for every data write queued before this call. Cancelling this observer
	// does not cancel or otherwise alter those writes.
	template <typename Token = use_sync_t>
	auto wait_written(Token &&token = {})
		requires task_token_v<Token>;

public:
	// Observes the next incoming Ping or Pong parsed by read() or close(). It does
	// not start a second transport read. With automatic_pong enabled, receiving
	// Ping queues the corresponding Pong before this operation completes. When
	// disabled, the latest unobserved Ping is retained for the application. The
	// synchronous form only consumes an already parsed event and otherwise
	// reports std::errc::operation_would_block.
	template <typename Token = use_sync_t>
	[[nodiscard]] auto wait_ctrl(Token &&token = {})
		requires task_token_v<Token,control_event_t>;

	template <typename Token = use_sync_t>
	auto ping(Token &&token = {})
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto ping(const const_buffer &payload, Token &&token = {})
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto pong(Token &&token = {})
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto pong(const const_buffer &payload, Token &&token = {})
		requires task_token_v<Token,size_t>;

	// Repeated asynchronous calls join an in-progress close. After the stream is
	// closed they complete immediately and successfully with the retained
	// close_info. A synchronous call cannot drive an executor-owned close already
	// in progress and reports std::errc::operation_in_progress instead.
	template <typename Token = use_sync_t>
	auto close(Token &&token = {})
		requires completion_token_v<Token,close_info_t>;

	template <typename Token = use_sync_t>
	auto close(close_frame_t frame, Token &&token = {})
		requires completion_token_v<Token,close_info_t>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto wait_closed(Token &&token = {})
		requires task_token_v<Token,close_info_t>;

public:
	[[nodiscard]] connection_state state() const noexcept;
	[[nodiscard]] bool is_open() const noexcept;
	[[nodiscard]] bool is_closing() const noexcept;
	[[nodiscard]] role stream_role() const noexcept;
	[[nodiscard]] std::string negotiated_subprotocol() const;
	[[nodiscard]] std::vector<extension> negotiated_extensions() const;

	[[nodiscard]] config_t config() const noexcept;
	[[nodiscard]] std::optional<close_info_t> peer_close() const;

	[[nodiscard]] http::endpoint remote_endpoint() const noexcept;
	[[nodiscard]] http::endpoint local_endpoint() const noexcept;
	[[nodiscard]] executor_t get_executor() const noexcept;

	basic_stream &cancel();
	basic_stream &cancel(error_code &error) noexcept;
	basic_stream &shutdown();
	basic_stream &shutdown(error_code &error) noexcept;

private:
	using impl = detail::stream_impl<Exec>;
	std::shared_ptr<impl> m_impl;
};

using stream = basic_stream<>;

} //namespace libgs::websocket
#include <libgs/websocket/detail/stream.h>


#endif //LIBGS_WEBSOCKET_STREAM_H
