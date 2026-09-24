# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.utils)
add_project(${target_name} gs.coro)

target_link_libraries(${target_name} PRIVATE libgs.spdlog)
target_compile_definitions(${target_name} PRIVATE
	LIBGS_UTILS_SHARED
	SPDLOG_COMPILED_LIB
	SPDLOG_USE_STD_FORMAT
)
# Keep a single spdlog implementation in gs.utils instead of instantiating its
# header-only implementation in logger.cpp.  Shared builds export the spdlog
# symbols from the utils library; static builds resolve them from the archive.
if (NOT LIBGS_BUILD_STATIC)
	target_compile_definitions(${target_name} PRIVATE
		SPDLOG_SHARED_LIB
		spdlog_EXPORTS
	)
endif ()

if (WIN32)
	target_compile_definitions(${target_name} PRIVATE SPDLOG_WCHAR_FILENAMES)
endif ()

option(LIBGS_BUILD_UTILITIES_SBUS_UDP
	"-- ${PRO_NAME}: Build Utilities.SoftBus <UDP> interface." ON
)
set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE "local" CACHE STRING
	"Select the Utilities.SoftBus default interface. (Default: local)"
)
set_property(CACHE LIBGS_UTILS_SBUS_DEFAULT_INTERFACE
	PROPERTY STRINGS local udp
)
string(TOLOWER "${LIBGS_UTILS_SBUS_DEFAULT_INTERFACE}"
	LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_NORMALIZED
)
if (LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_NORMALIZED STREQUAL "default")
	set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_NORMALIZED local)
endif ()

if (NOT LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_NORMALIZED MATCHES "^(local|udp)$")
	message(FATAL_ERROR
		"${PRO_NAME}: Unknown Utilities.SoftBus interface: "
		"${LIBGS_UTILS_SBUS_DEFAULT_INTERFACE}"
	)
endif ()

set(LIBGS_UTILS_SBUS_DEFAULT_INTERFACE
	"${LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_NORMALIZED}" CACHE STRING
	"Select the Utilities.SoftBus default interface. (Default: local)" FORCE
)
if (LIBGS_BUILD_UTILITIES AND
	LIBGS_UTILS_SBUS_DEFAULT_INTERFACE STREQUAL "udp" AND
	NOT LIBGS_BUILD_UTILITIES_SBUS_UDP)
	message(FATAL_ERROR
		"${PRO_NAME}: Unsupported Utilities.SoftBus interface: "
		"${LIBGS_UTILS_SBUS_DEFAULT_INTERFACE}"
	)
endif ()

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
