# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.websocket)
add_project(${target_name} gs.http)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_WEBSOCKET_SHARED
)
if (LIBGS_HTTP_ZLIB_SUPPORT)
	set(LIBGS_WEBSOCKET_ZLIB_SUPPORT ON)
	message(STATUS
		"${PRO_NAME}: WebSocket inherits z-lib support from the HTTP module."
	)
else ()
	option(LIBGS_WEBSOCKET_ZLIB_SUPPORT
		"-- ${PRO_NAME}: WebSocket permessage-deflate support." OFF
	)
endif ()

if (LIBGS_WEBSOCKET_ZLIB_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable WebSocket permessage-deflate support.")
	find_package(ZLIB REQUIRED)

	target_compile_definitions(${target_name}
		PUBLIC LIBGS_WEBSOCKET_ZLIB_SUPPORT=1
	)
	target_link_libraries(${target_name} PRIVATE ZLIB::ZLIB)
endif ()

set(LIBGS_WEBSOCKET_CONFIG_INCLUDE
	${LIBGS_OUTPUT_DIR}/config_include
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/websocket/cxx/configs.h.in
	${LIBGS_WEBSOCKET_CONFIG_INCLUDE}/libgs/websocket/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_WEBSOCKET_CONFIG_INCLUDE})

install(FILES
	${LIBGS_WEBSOCKET_CONFIG_INCLUDE}/libgs/websocket/cxx/configs.h
	DESTINATION include/libgs/websocket/cxx
)
if (WIN32)
	target_link_libraries(${target_name} PRIVATE bcrypt)
endif ()
