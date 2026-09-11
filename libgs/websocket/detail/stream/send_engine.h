// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_H

#include <libgs/websocket/detail/stream/send_operations.h>

namespace libgs::websocket::detail
{

// Owns the complete outbound state machine. Owner remains the single lifetime
// anchor and supplies transport/lifecycle hooks; the engine itself is a value
// member and never participates in shared ownership.
template <typename Owner>
class LIBGS_WEBSOCKET_TAPI send_engine
{
public:
	explicit send_engine(Owner &owner) noexcept;

	void reset(role local_role, const stream_config &config,
		std::span<const extension> extensions
	) noexcept;

	[[nodiscard]] sys_expected<prepared_frame> prepare_control (
		opcode op, const const_buffer &payload, bool borrow_payload = false
	) const noexcept;

	[[nodiscard]] sys_expected<prepared_frame> prepare_close(const close_frame &frame) const noexcept;

	[[nodiscard]] sys_expected<std::vector<prepared_frame>> prepare_message (
		message_type type, std::span<const const_buffer> buffers
	) const noexcept;

	[[nodiscard]] bool busy() const noexcept;
	[[nodiscard]] bool wire_write_active() const noexcept;
	[[nodiscard]] bool current_data_active() const noexcept;
	[[nodiscard]] bool ready_for_sync_protocol_write() const noexcept;

	void fail_current_if_idle(error_code error) noexcept;
	void fail_queued_controls(error_code error);
	void fail_queued_writes(error_code error);

	void clear_automatic_pong() noexcept;
	void clear_local_close() noexcept;
	void clear_close_response() noexcept;
	void clear_protocol_close() noexcept;
	void clear_protocol_frames() noexcept;

	void queue_local_close(prepared_frame frame);
	void queue_protocol_close(prepared_frame frame);
	[[nodiscard]] bool has_local_close() const noexcept;

	[[nodiscard]] sys_expected<> queue_close_response(const std::vector<std::byte> &payload) noexcept;
	[[nodiscard]] sys_expected<> queue_automatic_pong(const std::vector<std::byte> &payload) noexcept;

	void schedule();

	template <typename Handler>
	void async_write_message(message_type type, std::span<const const_buffer> buffers,
		std::shared_ptr<std::vector<std::byte>> payload_owner, Handler &&handler
	);

	template <typename Handler>
	void async_write_control(opcode op, const const_buffer &payload, Handler &&handler);

	[[nodiscard]] size_t write_control (
		opcode op, const const_buffer &payload, error_code &error
	) noexcept;

	void wait_written(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_written(Handler &&handler);

	// Transport is owned by stream::impl.  The engine only records the active
	// frame and consumes its completion to advance the outbound state machine.
	void complete_wire_frame(const prepared_frame &frame, wire_frame_kind kind,
		std::shared_ptr<send_operation> operation, error_code error, size_t wire_size
	) noexcept;

private:
	[[nodiscard]] bool queue_has_capacity(send_kind kind, size_t payload_size) const noexcept;
	void remember_write_error(uint64_t sequence, error_code error) noexcept;

	[[nodiscard]] error_code observe_write_error(uint64_t target) noexcept;
	void complete_write_waiters();

	void deliver_write_waiter(const std::shared_ptr<write_waiter> &waiter,
		error_code error, bool clear_slot = true
	) noexcept;

	void cancel_write_waiter(uint64_t id) noexcept;

	void deliver_send_completion(const std::shared_ptr<send_operation> &operation,
		error_code error
	) noexcept;

	void complete_send_operation(const std::shared_ptr<send_operation> &operation,
		error_code error
	) noexcept;

	void install_send_cancellation(const std::shared_ptr<send_operation> &operation);
	void cancel_queued_send(uint64_t id) noexcept;

	void start_wire_frame(prepared_frame frame, wire_frame_kind kind,
		std::shared_ptr<send_operation> operation = {}
	) noexcept;

	[[nodiscard]] error_code enqueue_send_operation(
		std::shared_ptr<send_operation> operation
	) noexcept;

	[[nodiscard]] static sys_expected<> retain_protocol_payload (
		optional<std::vector<std::byte>> &slot, const std::vector<std::byte> &payload
	) noexcept;

private:
	Owner &m_owner;
	frame_builder m_frame_builder {};
	size_t m_max_queued_write_bytes = 0;
	size_t m_max_queued_write_operations = 0;

	optional<std::vector<std::byte>> m_pending_auto_pong {};
	optional<prepared_frame> m_pending_local_close {};

	optional<std::vector<std::byte>> m_pending_close_response {};
	optional<prepared_frame> m_pending_protocol_close {};

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
#include <libgs/websocket/detail/stream/send_engine.ipp>


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_ENGINE_H
