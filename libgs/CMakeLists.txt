# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

include(project)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES core.h)
add_subdirectory(core)

if (LIBGS_BUILD_CORO)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES coro.h)
	add_subdirectory(coro)
endif ()

if (LIBGS_BUILD_HTTP)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES http.h)
	add_subdirectory(http)
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES websocket.h)
	add_subdirectory(websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES utils.h)
	add_subdirectory(utils)
endif ()
