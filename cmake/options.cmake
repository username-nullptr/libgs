# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

option(LIBGS_BUILD_STATIC
	"-- ${PRO_NAME}: Build static libraries." ${libgs_build_static_default}
)
if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
	NOT libgs_gnu_shared_runtime_available AND NOT LIBGS_BUILD_STATIC)
	message(FATAL_ERROR
		"${PRO_NAME}: Shared libraries require a shared GNU C++ runtime on Windows. "
		"Use -DLIBGS_BUILD_STATIC=ON or a MinGW toolchain that provides libstdc++-6.dll."
	)
endif ()

option(LIBGS_ADD_LIBRARY_VERSION
	"-- ${PRO_NAME}: Add version information to shared library names." ON
)
option(LIBGS_OPENSSL_SUPPORT
	"-- ${PRO_NAME}: OpenSSL support." OFF
)
if (LIBGS_OPENSSL_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable OpenSSL support.")
	add_definitions(-DLIBGS_OPENSSL_SUPPORT=1)
endif ()

option(LIBGS_IO_URING_SUPPORT
	"-- ${PRO_NAME}: Linux I/O uring support." OFF
)
if (LIBGS_IO_URING_SUPPORT)
	if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
		message(FATAL_ERROR
			"${PRO_NAME}: LIBGS_IO_URING_SUPPORT requires Linux."
		)
	endif ()
	message(STATUS "${PRO_NAME}: Enable Linux I/O uring support.")
endif ()

option(LIBGS_BUILD_CORO
	"-- ${PRO_NAME}: Build module <Coroutine>." ON
)
if (LIBGS_BUILD_CORO)
	message(STATUS "${PRO_NAME}: Build module <Coroutine>.")
endif ()

option(LIBGS_BUILD_HTTP
	"-- ${PRO_NAME}: Build module <HTTP>." OFF
)
if (LIBGS_BUILD_HTTP)
	if (NOT LIBGS_BUILD_CORO)
		message(FATAL_ERROR "${PRO_NAME}: <HTTP> module depends on <Coroutine> module.")
		unset(LIBGS_BUILD_HTTP)
	elseif (LIBGS_OPENSSL_SUPPORT)
		message(STATUS "${PRO_NAME}: Build module <HTTP/HTTPS>.")
	else ()
		message(STATUS "${PRO_NAME}: Build module <HTTP>.")
	endif ()
endif ()

option(LIBGS_BUILD_WEBSOCKET
	"-- ${PRO_NAME}: Build module <WebSocket>." OFF
)
if (LIBGS_BUILD_WEBSOCKET)
	if (NOT LIBGS_BUILD_HTTP)
		message(FATAL_ERROR "${PRO_NAME}: <WebSocket> module depends on <HTTP> module.")
		unset(LIBGS_BUILD_WEBSOCKET)
	else ()
		message(STATUS "${PRO_NAME}: Build module <WebSocket>.")
	endif ()
endif ()

option(LIBGS_BUILD_UTILITIES
	"-- ${PRO_NAME}: Build module <Utilities>." OFF
)
if (LIBGS_BUILD_UTILITIES)
	if (NOT LIBGS_BUILD_CORO)
		message(FATAL_ERROR "${PRO_NAME}: <Utilities> module depends on <Coroutine> module.")
		unset(LIBGS_BUILD_UTILITIES)
	else ()
		message(STATUS "${PRO_NAME}: Build module <Utilities>.")
	endif ()
endif ()

option(LIBGS_HTTP_ZLIB_SUPPORT
	"-- ${PRO_NAME}: HTTP z-lib support." OFF
)
option(LIBGS_WEBSOCKET_ZLIB_SUPPORT
	"-- ${PRO_NAME}: WebSocket permessage-deflate support." OFF
)
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

option(LIBGS_BUILD_EXAMPLES
	"-- ${PRO_NAME}: Enable this to build the examples." OFF
)
set(LIBGS_CORO_SUPPORT ${LIBGS_BUILD_CORO})
set(LIBGS_HTTP_SUPPORT ${LIBGS_BUILD_HTTP})

set(LIBGS_WEBSOCKET_SUPPORT ${LIBGS_BUILD_WEBSOCKET})
set(LIBGS_UTILITIES_SUPPORT ${LIBGS_BUILD_UTILITIES})

set(LIBGS_CONFIG_INCLUDE
	${LIBGS_OUTPUT_DIR}/config_include
)
set(LIBGS_CONFIG_INCLUDE
	${LIBGS_CONFIG_INCLUDE} CACHE PATH
	"Path to libgs config include directory."
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/core/cxx/configs.h.in
	${LIBGS_CONFIG_INCLUDE}/libgs/core/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_CONFIG_INCLUDE})

install(FILES
	${LIBGS_CONFIG_INCLUDE}/libgs/core/cxx/configs.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libgs/core/cxx
)
