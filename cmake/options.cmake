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

set(LIBGS_OPENSSL_INSTALL_PREFIX "" CACHE PATH
	"Install prefix of an external OpenSSL package."
)
set(LIBGS_ZLIB_INSTALL_PREFIX "" CACHE PATH
	"Install prefix of an external zlib package."
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
set(LIBGS_CORO_SUPPORT ${LIBGS_BUILD_CORO})

if (LIBGS_BUILD_CORO)
	message(STATUS "${PRO_NAME}: Build module <Coroutine>.")
endif ()

option(LIBGS_BUILD_HTTP
	"-- ${PRO_NAME}: Build module <HTTP>." OFF
)
set(LIBGS_HTTP_SUPPORT ${LIBGS_BUILD_HTTP})

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
set(LIBGS_WEBSOCKET_SUPPORT ${LIBGS_BUILD_WEBSOCKET})

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
set(LIBGS_UTILITIES_SUPPORT ${LIBGS_BUILD_UTILITIES})

if (LIBGS_BUILD_UTILITIES)
	if (NOT LIBGS_BUILD_CORO)
		message(FATAL_ERROR "${PRO_NAME}: <Utilities> module depends on <Coroutine> module.")
		unset(LIBGS_BUILD_UTILITIES)
	else ()
		message(STATUS "${PRO_NAME}: Build module <Utilities>.")
	endif ()
endif ()

option(LIBGS_BUILD_EXAMPLES
	"-- ${PRO_NAME}: Enable this to build the examples." OFF
)
if (LIBGS_BUILD_EXAMPLES)
	message(STATUS "${PRO_NAME}: Enable this to build the examples.")
endif()

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
