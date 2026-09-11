// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_OPERATIONS_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_OPERATIONS_H

#include <libgs/websocket/types.h>

namespace libgs::websocket::detail
{

struct control_wait_operation
{
	uint64_t id = 0;
	asio::any_completion_handler<void(error_code,control_event)> completion {};
};

struct read_wait_operation
{
	uint64_t id = 0;
	asio::any_completion_handler<void(error_code,message)> completion {};
	asio::cancellation_signal cancellation {};
};

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_RECEIVE_OPERATIONS_H
