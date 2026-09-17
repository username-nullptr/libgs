# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

add_libgs_stress_test(core core.cpp gs.core)

if (LIBGS_BUILD_CORO)
	add_libgs_stress_test(coro coro.cpp gs.coro)
endif ()

if (LIBGS_BUILD_HTTP)
	add_libgs_stress_test(http http.cpp gs.http)
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	add_libgs_stress_test(websocket websocket.cpp gs.websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	add_libgs_stress_test(utils utils.cpp gs.utils)
endif ()
