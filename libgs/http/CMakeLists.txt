# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.http)
add_project(${target_name} gs.coro)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_HTTP_SHARED
)
option(LIBGS_HTTP_ZLIB_SUPPORT
	"-- ${PRO_NAME}: HTTP z-lib support." OFF
)
if (LIBGS_HTTP_ZLIB_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable z-lib support.")
	add_definitions(-DLIBGS_HTTP_ZLIB_SUPPORT=1)

	find_package(ZLIB REQUIRED)
	target_link_libraries(${target_name} PRIVATE ZLIB::ZLIB)
endif ()

set(LIBGS_HTTP_CONFIG_INCLUDE
	${LIBGS_OUTPUT_DIR}/config_include
)
set(LIBGS_HTTP_CONFIG_INCLUDE
	${LIBGS_HTTP_CONFIG_INCLUDE} CACHE PATH
	"Path to libgs config include directory."
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/http/cxx/configs.h.in
	${LIBGS_HTTP_CONFIG_INCLUDE}/libgs/http/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_HTTP_CONFIG_INCLUDE})

install(FILES
	${LIBGS_HTTP_CONFIG_INCLUDE}/libgs/http/cxx/configs.h
	DESTINATION include/libgs/http/cxx
)
