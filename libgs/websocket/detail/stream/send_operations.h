// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_OPERATIONS_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_OPERATIONS_H

#include <libgs/websocket/detail/stream/frame_builder.h>

namespace libgs::websocket::detail
{

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

	asio::any_completion_handler<void(error_code)> completion {};
};

enum class wire_frame_kind : uint8_t
{
	data, application_control, automatic_pong,
	local_close, close_response, protocol_close,
};

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_SEND_OPERATIONS_H
