// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_OPERATIONS_H
#define LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_OPERATIONS_H

#include <libgs/websocket/types.h>

namespace libgs::websocket::detail
{

struct close_wait_operation
{
	uint64_t id = 0;
	asio::any_completion_handler<void(error_code,close_info)> completion {};
};

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_CLOSE_OPERATIONS_H
