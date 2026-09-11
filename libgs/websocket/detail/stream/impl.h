// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H

#include <libgs/websocket/detail/secure_random.h>
#include <libgs/websocket/protocol/detail/utf8.h>
#include <libgs/websocket/protocol/generator.h>
#include <libgs/websocket/protocol/parser.h>

namespace libgs::websocket::detail
{

template <core_concepts::exec Exec>
class LIBGS_WEBSOCKET_TAPI stream_impl :
	public std::enable_shared_from_this<stream_impl<Exec>>
{
public:
	using executor_t = Exec;
	using config_t = stream_config;

	using connection_t = http::basic_connection<executor_t>;
	using connection_ptr = std::shared_ptr<connection_t>;

	using adopt_options_t = adopt_options;
	using close_info_t = close_info;

	using control_event_t = control_event;

	struct prepared_frame
	{
		std::shared_ptr<std::vector<std::byte>> wire;
		std::vector<const_buffer> buffers;

		size_t header_size = 0;
		size_t payload_size = 0;
	};

	enum class send_kind : uint8_t {
		data, application_control,
	};

	struct send_operation
	{
		send_kind kind = send_kind::data;
		std::vector<prepared_frame> frames {};

		std::shared_ptr<std::vector<std::byte>> payload_owner {};
		asio::any_completion_handler<void(error_code,size_t)> completion {};

		size_t frame_index = 0;
		size_t transferred = 0;
		size_t queued_payload_size = 0;

		uint64_t sequence = 0;
		uint64_t id = 0;

		bool cancel_requested = false;
		bool queued_counted = false;
	};

	struct write_waiter
	{
		uint64_t id = 0;
		uint64_t target = 0;

		asio::any_completion_handler <
			void(error_code)
		> completion {};
	};

	struct control_wait_operation
	{
		uint64_t id = 0;
		asio::any_completion_handler <
			void(error_code,control_event_t)
		> completion {};
	};

	struct read_wait_operation
	{
		uint64_t id = 0;

		asio::any_completion_handler <
			void(error_code,message)
		> completion {};

		asio::cancellation_signal cancellation {};
	};

	struct close_wait_operation
	{
		uint64_t id = 0;
		asio::any_completion_handler <
			void(error_code,close_info_t)
		> completion {};
	};

	enum class wire_frame_kind : uint8_t
	{
		data, application_control, automatic_pong,
		local_close, close_response,
		protocol_close,
	};

	struct received_event
	{
		opcode op = opcode::binary;
		optional<message> data {};
		std::vector<std::byte> control {};
	};

	explicit stream_impl(executor_t exec, config_t config);

public:
	void adopt(connection_ptr connection,
		adopt_options_t options, error_code &error
	) noexcept;

	[[nodiscard]] size_t write(message_type type,
		std::span<const const_buffer> buffers, error_code &error
	) noexcept;

	[[nodiscard]] sys_expected<prepared_frame> prepare_control_frame (
		opcode op, const const_buffer &payload, bool borrow_payload = false
	) const noexcept;

	[[nodiscard]] sys_expected<std::vector<prepared_frame>> prepare_frames (
		message_type type, std::span<const const_buffer> buffers
	) const noexcept;

	[[nodiscard]] size_t write_prepared (
		const prepared_frame &frame, error_code &error
	) noexcept;

	[[nodiscard]] bool send_engine_busy() const noexcept;
	[[nodiscard]] error_code state_write_error() const noexcept;
	[[nodiscard]] bool queue_has_capacity(send_kind kind, size_t payload_size) const noexcept;

	void remember_write_error(uint64_t sequence, error_code error) noexcept;
	[[nodiscard]] error_code observe_write_error(uint64_t target) noexcept;

	void complete_write_waiters();

	void deliver_write_waiter(std::shared_ptr<write_waiter> waiter,
		error_code error, bool clear_slot = true
	) noexcept;

	void cancel_write_waiter(uint64_t id) noexcept;

	void deliver_send_completion (
		const std::shared_ptr<send_operation> &operation, error_code error
	) noexcept;

	void complete_send_operation (
		const std::shared_ptr<send_operation> &operation, error_code error
	) noexcept;

	void install_send_cancellation (
		const std::shared_ptr<send_operation> &operation
	);

	void cancel_queued_send(uint64_t id) noexcept;
	void fail_queued_controls(error_code error);
	void fail_queued_writes(error_code error);

	void start_wire_frame(prepared_frame frame,
		wire_frame_kind kind, std::shared_ptr<send_operation> operation = {}
	) noexcept;

	void schedule_send();

	[[nodiscard]] error_code enqueue_send_operation (
		std::shared_ptr<send_operation> operation
	) noexcept;

	template <typename Handler>
	void async_write_message(message_type type, std::span<const const_buffer> buffers,
		std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler
	);

	template <typename Handler>
	void async_write_control (
		opcode op, const const_buffer &payload, Handler &&handler
	);

	[[nodiscard]] size_t write_control (
		opcode op, const const_buffer &payload, error_code &error
	) noexcept;

	void wait_written(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_written(Handler &&handler);

	[[nodiscard]] sys_expected<> retain_protocol_payload (
		optional<std::vector<std::byte>> &slot, const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] sys_expected<> queue_automatic_pong (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] sys_expected<> begin_peer_close (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] sys_expected<> remember_peer_close (
		const std::vector<std::byte> &payload
	) noexcept;

	[[nodiscard]] error_code control_state_error() const noexcept;

	void complete_control_waiter (
		error_code error, control_event_t event = {}, bool clear_slot = true
	) noexcept;

	void cancel_control_waiter(uint64_t id) noexcept;
	void stop_control_observer(error_code error) noexcept;

	void remember_control(opcode op, std::vector<std::byte> payload) noexcept;
	[[nodiscard]] control_event_t wait_control(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_control(Handler &&handler);

	[[nodiscard]] mutable_buffer available_read_data() noexcept;
	void consume_read_data(size_t size) noexcept;

	[[nodiscard]] sys_expected<optional<received_event>>
	consume_frame_data() noexcept;

	void close_transport(error_code &error) noexcept;

	[[nodiscard]] message finish_read_error(error_code &error) noexcept;
	[[nodiscard]] message read(error_code &error) noexcept;

	template <typename Buffer>
	[[nodiscard]] static basic_message<Buffer> convert_message (
		message value, error_code &error
	) noexcept;

	template <typename Handler>
	void async_read_message(Handler &&handler);

	void complete_read_waiter (
		error_code error, message value = {}, bool clear_slot = true
	) noexcept;

	void cancel_read_waiter (
		uint64_t id, asio::cancellation_type type
	) noexcept;

	[[nodiscard]] close_info_t retained_close_info(bool clean = false) const;

	void complete_close_waiters(error_code error) noexcept;
	void cancel_close_waiter(uint64_t id) noexcept;
	void start_close_deadline() noexcept;

	void finish_close (
		error_code error, bool clean, bool cancel_transport = false
	) noexcept;

	void start_close_receive() noexcept;
	void begin_local_close(prepared_frame frame) noexcept;

	[[nodiscard]] close_info_t close(const close_frame &frame, error_code &error) noexcept;
	[[nodiscard]] close_info_t wait_closed(error_code &error) noexcept;

	template <typename Handler>
	void async_close(close_frame frame, Handler &&handler);

	template <typename Handler>
	void async_wait_closed(Handler &&handler);

	template <typename Handler>
	bool add_close_waiter(Handler &&handler) noexcept;

	void cancel(error_code &error) noexcept;
	void shutdown(error_code &error) noexcept;

	[[nodiscard]] static optional<close_code>
	protocol_failure_code(error_code error) noexcept;

	void begin_protocol_failure(error_code error, bool synchronous = false) noexcept;
	void finish_protocol_failure(bool cancel_transport = false) noexcept;
	void fail(error_code error) noexcept;

public:
	executor_t m_exec {};
	config_t m_config {};

	connection_ptr m_connection;
	connection_state m_state = connection_state::idle;

	role m_role = role::client;
	std::vector<std::byte> m_pending_data {};

	size_t m_pending_offset = 0;
	std::unique_ptr<frame_parser> m_parser {};
	std::shared_ptr<std::vector<std::byte>> m_read_buffer {};

	size_t m_read_size = 0;
	size_t m_read_offset = 0;

	error_code m_read_error {};
	bool m_read_active = false;

	std::shared_ptr<read_wait_operation> m_read_waiter {};
	uint64_t m_next_read_waiter_id = 0;

	optional<message_type> m_message_type {};
	std::vector<std::byte> m_message_body {};
	std::vector<std::byte> m_control_body {};

	optional<control_event_t> m_control_event {};
	std::shared_ptr<control_wait_operation> m_control_waiter;

	uint64_t m_next_control_waiter_id = 0;
	optional<std::vector<std::byte>> m_pending_auto_pong {};
	optional<prepared_frame> m_pending_local_close {};

	optional<std::vector<std::byte>> m_pending_close_response {};
	optional<prepared_frame> m_pending_protocol_close {};

	std::string m_subprotocol {};
	std::vector<extension> m_extensions {};

	optional<close_info_t> m_peer_close {};
	optional<close_info_t> m_close_result {};

	std::shared_ptr<asio::steady_timer> m_close_timer {};
	std::deque<std::shared_ptr<close_wait_operation>> m_close_waiters {};

	uint64_t m_next_close_waiter_id = 0;
	bool m_local_close_initiated = false;
	bool m_local_close_sent = false;

	bool m_protocol_failure_active = false;
	error_code m_error {};

	bool m_transport_closed = false;
	bool m_wire_write_active = false;
	bool m_last_wire_was_control = false;

	std::shared_ptr<send_operation> m_current_data {};
	std::deque<std::shared_ptr<send_operation>> m_data_write_queue {};
	std::deque<std::shared_ptr<send_operation>> m_control_write_queue {};

	size_t m_queued_write_operations = 0;
	size_t m_queued_write_bytes = 0;

	uint64_t m_last_write_sequence = 0;
	uint64_t m_completed_write_sequence = 0;
	uint64_t m_next_send_operation_id = 0;

	optional<std::pair<uint64_t,error_code>> m_unobserved_write_error {};
	std::deque<std::shared_ptr<write_waiter>> m_write_waiters {};
	uint64_t m_next_write_waiter_id = 0;
};

} //namespace libgs::websocket::detail

#include <libgs/websocket/detail/stream/transport.ipp>
#include <libgs/websocket/detail/stream/lifecycle.ipp>
#include <libgs/websocket/detail/stream/frame_io.ipp>
#include <libgs/websocket/detail/stream/control.ipp>
#include <libgs/websocket/detail/stream/receive.ipp>
#include <libgs/websocket/detail/stream/close.ipp>
#include <libgs/websocket/detail/stream/send.ipp>


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_IMPL_H
