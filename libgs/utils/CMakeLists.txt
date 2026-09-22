# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.utils)
add_project(${target_name} gs.coro)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_UTILS_SHARED
)
set(LIBGS_UTILS_SBUS_UDP_INTERFACE_SUPPORT
	${LIBGS_BUILD_UTILITIES_SBUS_UDP}
)
if (LIBGS_BUILD_UTILITIES_SBUS_UDP)
	message(STATUS "${PRO_NAME}: Build Utilities.SoftBus <UDP> interface.")
	target_compile_definitions(${target_name} PUBLIC
		LIBGS_UTILS_SBUS_UDP_INTERFACE_SUPPORT=1
	)
endif ()

set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_LOCAL 0)
set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_UDP 0)

if (LIBGS_UTILS_SBUS_DEFAULT_INTERFACE STREQUAL "udp")
	set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_UDP 1)
else ()
	set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_LOCAL 1)
endif ()

set(LIBGS_UTILS_CONFIG_INCLUDE
	${LIBGS_OUTPUT_DIR}/config_include
)
set(LIBGS_UTILS_CONFIG_INCLUDE
	${LIBGS_UTILS_CONFIG_INCLUDE} CACHE PATH
	"Path to libgs config include directory."
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/utils/cxx/configs.h.in
	${LIBGS_UTILS_CONFIG_INCLUDE}/libgs/utils/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_UTILS_CONFIG_INCLUDE})

install(FILES
	${LIBGS_UTILS_CONFIG_INCLUDE}/libgs/utils/cxx/configs.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libgs/utils/cxx
)
