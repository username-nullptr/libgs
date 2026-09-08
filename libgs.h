// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_LIBGS_H
#define LIBGS_LIBGS_H

#include <libgs/core.h>

#if LIBGS_CORO_SUPPORT
# include <libgs/coro.h>
#endif

#if LIBGS_HTTP_SUPPORT
# include <libgs/http.h>
#endif

#if LIBGS_WEBSOCKET_SUPPORT
# include <libgs/websocket.h>
#endif

#if LIBGS_UTILITIES_SUPPORT
# include <libgs/utils.h>
#endif


#endif //LIBGS_LIBGS_H
