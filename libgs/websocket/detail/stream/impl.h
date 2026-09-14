// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H

#ifndef LIBGS_WEBSOCKET_STREAM_H
# error "Include <libgs/websocket/stream.h> instead."
#endif

#include <libgs/websocket/detail/stream/close_operations.h>
#include <libgs/websocket/detail/stream/receive_engine.h>
#include <libgs/websocket/detail/stream/send_engine.h>

namespace libgs::websocket
{

template <core_concepts::exec Exec>
class LIBGS_WEBSOCKET_TAPI basic_stream<Exec>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	friend class basic_stream;

	enum class local_close_phase : uint8_t {
		none, queued, sent,
	};

public:
	using adopt_options_t = adopt_options;
	using close_info_t    = close_info   ;
	using control_event_t = control_event;
	using prepared_frame  = detail::prepared_frame;
	using close_wait_operation = detail::close_wait_operation;

	explicit impl(executor_t exec, const config_t &config);

public:
	void adopt(connection_ptr connection,
		adopt_options_t options, error_code &error
	) noexcept;

	[[nodiscard]] size_t write(message_type type,
		std::span<const const_buffer> buffers, error_code &error
	) noexcept;

	[[nodiscard]] size_t write_frame(message_type type,
		const const_buffer &payload, bool continuation,
		bool fin, error_code &error
	) noexcept;

	template <typename Handler>
	void async_write_message(message_type type, std::span<const const_buffer> buffers,
		std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler
	);

	template <typename Handler>
	void async_write_frame(message_type type, const const_buffer &payload,
		bool continuation, bool fin, std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler
	);
	[[nodiscard]] size_t write_control (
		opcode op, const const_buffer &payload, error_code &error
	) noexcept;

	template <typename Handler>
	void async_write_control (
		opcode op, const const_buffer &payload, Handler &&handler
	);

	void wait_written(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_written(Handler &&handler);

	[[nodiscard]] message read(error_code &error) noexcept;
	[[nodiscard]] data_frame read_frame(error_code &error) noexcept;

	template <typename Handler>
	void async_read_message(Handler &&handler);

	template <typename Handler>
	void async_read_frame(Handler &&handler);

	template <typename Consumer>
	[[nodiscard]] message_info consume(Consumer &&consumer, error_code &error) noexcept;

	template <typename Consumer, typename Handler>
	void async_consume(Consumer &&consumer, Handler &&handler);

	template <typename Buffer>
	[[nodiscard]] static basic_message<Buffer> convert_message (
		message value, error_code &error
	) noexcept;

	template <typename Buffer>
	[[nodiscard]] static basic_data_frame<Buffer> convert_frame (
		data_frame value, error_code &error
	) noexcept;

	[[nodiscard]] control_event_t wait_control(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_control(Handler &&handler);

	[[nodiscard]] close_info_t close(const close_frame &frame, error_code &error) noexcept;

	template <typename Handler>
	void async_close(close_frame frame, Handler &&handler);

	[[nodiscard]] close_info_t wait_closed(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_closed(Handler &&handler);

	void cancel(error_code &error) noexcept;
	void shutdown(error_code &error) noexcept;

public:
	[[nodiscard]] detail::receive_engine<impl> &receive_side() noexcept;
	[[nodiscard]] detail::send_engine<impl> &send_side() noexcept;

	[[nodiscard]] size_t write_prepared (
		const prepared_frame &frame, error_code &error
	) noexcept;
	[[nodiscard]] size_t read_transport (
		const mutable_buffer &buffer, error_code &error
	) noexcept;

	[[nodiscard]] awaitable<std::tuple<error_code,size_t>>
	async_read_transport(std::shared_ptr<std::vector<std::byte>> storage);

	[[nodiscard]] executor_t executor() const noexcept;

	[[nodiscard]] error_code write_state_error() const noexcept;
	[[nodiscard]] error_code read_state_error() const noexcept;

	[[nodiscard]] error_code frame_read_state_error() const noexcept;
	[[nodiscard]] error_code frame_write_state_error() const noexcept;

	[[nodiscard]] error_code consume_state_error() const noexcept;
	[[nodiscard]] error_code control_state_error() const noexcept;

	[[nodiscard]] bool send_transport_ready() const noexcept;
	[[nodiscard]] bool automatic_pong_enabled() const noexcept;

	[[nodiscard]] bool close_receive_pending() const noexcept;
	[[nodiscard]] bool protocol_failure_active() const noexcept;

	[[nodiscard]] error_code protocol_failure_error(error_code fallback) const noexcept;
	[[nodiscard]] error_code finish_receive_eof() noexcept;

	[[nodiscard]] sys_expected<> handle_sync_ping (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] sys_expected<> handle_sync_peer_close (
		const std::vector<std::byte> &payload
	) noexcept;

	void start_transport_write(prepared_frame frame,
		detail::wire_frame_kind kind, std::shared_ptr<detail::send_operation> operation
	) noexcept;

	void handle_send_failure(error_code error) noexcept;
	void handle_wire_frame_sent(detail::wire_frame_kind kind) noexcept;

	void handle_receive_failure(error_code error) noexcept;
	void handle_receive_protocol_failure(error_code error, bool synchronous = false) noexcept;

	[[nodiscard]] sys_expected<> queue_automatic_pong (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] sys_expected<> begin_peer_close (
		const std::vector<std::byte> &payload
	) noexcept;
	void start_close_receive() noexcept;

private:
	void close_transport(error_code &error) noexcept;

	[[nodiscard]] bool local_close_started() const noexcept;
	[[nodiscard]] bool local_close_sent() const noexcept;

	void mark_local_close_queued() noexcept;
	void mark_local_close_sent() noexcept;

	[[nodiscard]] sys_expected<> remember_peer_close (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] close_info_t retained_close_info(bool clean = false) const;

	void begin_local_close(prepared_frame frame) noexcept;
	void start_close_deadline() noexcept;

	void finish_close(error_code error, bool clean,
		bool cancel_transport = false
	) noexcept;

	template <typename Handler>
	bool add_close_waiter(Handler &&handler) noexcept;

	void complete_close_waiters(error_code error) noexcept;
	void cancel_close_waiter(uint64_t id) noexcept;

	// Protocol and transport failure handling.
	[[nodiscard]] static optional<close_code>
	protocol_failure_code(error_code error) noexcept;

	void begin_protocol_failure(error_code error, bool synchronous = false) noexcept;
	void finish_protocol_failure(bool cancel_transport = false) noexcept;
	void fail(error_code error) noexcept;

private:
	executor_t m_exec {};
	config_t m_config {};

	connection_ptr m_connection;
	connection_state m_state = connection_state::idle;

	role m_role = role::client;
	std::string m_subprotocol {};
	std::vector<extension> m_extensions {};

	detail::receive_engine<impl> m_receive_engine;
	detail::send_engine<impl> m_send_engine;

	optional<close_info_t> m_peer_close {};
	optional<close_info_t> m_close_result {};

	std::shared_ptr<asio::steady_timer> m_close_timer {};
	std::deque<std::shared_ptr<close_wait_operation>> m_close_waiters {};

	uint64_t m_next_close_waiter_id = 0;
	local_close_phase m_local_close_phase = local_close_phase::none;

	// Terminal failure and transport state.
	bool m_protocol_failure_active = false;
	error_code m_error {};

	bool m_transport_closed = false;
};

} //namespace libgs::websocket
#include <libgs/websocket/detail/permessage_deflate.h>

#include <libgs/websocket/detail/stream/impl/core.ipp>
#include <libgs/websocket/detail/stream/impl/transport.ipp>

#include <libgs/websocket/detail/stream/impl/write.ipp>
#include <libgs/websocket/detail/stream/impl/read.ipp>

#include <libgs/websocket/detail/stream/impl/control.ipp>
#include <libgs/websocket/detail/stream/impl/close.ipp>

#include <libgs/websocket/detail/stream/impl/lifecycle.ipp>


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
