// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#if LIBGS_CONSUME_UMBRELLA
# include <libgs.h>
#endif

#if LIBGS_CONSUME_CORE
# include <libgs/core.h>
#endif
#if LIBGS_CONSUME_CORO
# include <libgs/coro.h>
#endif
#if LIBGS_CONSUME_HTTP
# include <libgs/http.h>
#endif
#if LIBGS_CONSUME_WEBSOCKET
# include <libgs/websocket.h>
#endif
#if LIBGS_CONSUME_UTILS
# include <libgs/utils.h>
#endif

#include <libgs/core/global.h>

#include <string_view>

int main()
{
	const std::string_view version = libgs::version_string();
	return version.empty() ? 1 : 0;
}
