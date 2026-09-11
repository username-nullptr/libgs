// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_H

#include <libgs/websocket/detail/stream/receive_operations.h>
#include <libgs/websocket/detail/stream/receive_buffer.h>
#include <libgs/core/async_expected.h>

namespace libgs::websocket::detail
{

enum class receive_failure_origin : uint8_t {
	none, transport, protocol, buffer,
};

struct receive_event_result
{
	optional<received_event> event {};
	error_code error {};

	receive_failure_origin failure_origin =
		receive_failure_origin::none;
};

// Owns receive-operation and control-observer state. Close-handshake code may
// temporarily drive the same event pump, while Owner remains responsible for
// transport lifetime and connection lifecycle.
template <typename Owner>
class LIBGS_WEBSOCKET_TAPI receive_engine
{
public:
	explicit receive_engine(Owner &owner) noexcept;

	void reset(role local_role, const stream_config &config,
		std::span<const extension> extensions, std::vector<std::byte> pending_data
	);
	[[nodiscard]] bool active() const noexcept;
	void set_active(bool value) noexcept;

	[[nodiscard]] receive_event_result next_event() noexcept;
	[[nodiscard]] awaitable<receive_event_result> async_next_event();

	[[nodiscard]] error_code control_state_error() const noexcept;

	void complete_control_waiter(error_code error,
		control_event event = {}, bool clear_slot = true
	) noexcept;

	void stop_control_observer(error_code error) noexcept;
	void remember_control(opcode op, std::vector<std::byte> payload) noexcept;

	[[nodiscard]] control_event wait_control(error_code &error) noexcept;

	template <typename Handler>
	void async_wait_control(Handler &&handler);

	[[nodiscard]] message read(error_code &error) noexcept;
	[[nodiscard]] data_frame read_frame(error_code &error) noexcept;

	template <typename Handler>
	void async_read_message(Handler &&handler);

	template <typename Handler>
	void async_read_frame(Handler &&handler);

	void complete_read_waiter(error_code error,
		message value = {}, bool clear_slot = true
	) noexcept;

	void complete_frame_read_waiter(error_code error,
		data_frame value = {}, bool clear_slot = true
	) noexcept;

private:
	[[nodiscard]] message finish_read_error(error_code &error) noexcept;

	void cancel_control_waiter(uint64_t id) noexcept;
	void cancel_read_waiter(uint64_t id, asio::cancellation_type type) noexcept;

private:
	Owner &m_owner;
	receive_buffer m_buffer {};

	error_code m_read_error {};
	bool m_read_active = false;

	std::shared_ptr<read_wait_operation> m_read_waiter {};
	std::shared_ptr<frame_read_wait_operation> m_frame_read_waiter {};
	uint64_t m_next_read_waiter_id = 0;

	optional<control_event> m_control_event {};
	std::shared_ptr<control_wait_operation> m_control_waiter {};

	uint64_t m_next_control_waiter_id = 0;
};

} //namespace libgs::websocket::detail

#include <libgs/websocket/detail/stream/receive_engine_control.ipp>
#include <libgs/websocket/detail/stream/receive_engine_read.ipp>


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_ENGINE_H
