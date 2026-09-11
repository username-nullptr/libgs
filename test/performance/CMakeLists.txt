# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

add_libgs_performance_test(lock-free-queue lock_free_queue.cpp gs.core)
add_libgs_performance_test(core core.cpp gs.core)

if (LIBGS_BUILD_CORO)
	add_libgs_performance_test(coro coro.cpp gs.core)
endif ()

if (LIBGS_BUILD_HTTP)
	add_libgs_performance_test(http http.cpp gs.http)
	add_libgs_performance_test(http-protocol http_protocol.cpp gs.http)
endif ()

if (LIBGS_BUILD_UTILITIES)
	add_libgs_performance_test(signal-slot signal_slot.cpp gs.utils)
	add_libgs_performance_test(sbus sbus.cpp gs.utils)
endif ()
