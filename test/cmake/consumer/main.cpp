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
# include <spdlog/spdlog.h>
#endif

#include <libgs/core/global.h>

#include <string_view>

int main()
{
	const std::string_view version = libgs::version_string();
	if(version.empty())
		return 1;

#if LIBGS_CONSUME_UTILS
	// gs.utils carries one compiled spdlog implementation.  Exercise a direct
	// imported symbol so shared-package tests verify the spdlog export/import
	// contract instead of only LibGS's logger wrapper.
	const auto original_level = spdlog::get_level();
	spdlog::set_level(spdlog::level::info);
	if(spdlog::get_level() != spdlog::level::info)
		return 2;
	spdlog::set_level(original_level);
#endif

	return 0;
}
