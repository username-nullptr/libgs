# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(libgs_build_static_default OFF)
set(libgs_gnu_shared_runtime_available TRUE)

if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	execute_process (
		COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++-6.dll
		OUTPUT_VARIABLE libgs_gnu_libstdcxx_dll
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
	if (NOT IS_ABSOLUTE "${libgs_gnu_libstdcxx_dll}" OR NOT EXISTS "${libgs_gnu_libstdcxx_dll}")
		set(libgs_gnu_shared_runtime_available FALSE)
		set(libgs_build_static_default ON)

		message(STATUS
			"${PRO_NAME}: GNU C++ runtime is static-only."
		)
	endif ()
endif ()

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

if (NOT LIBGS_BUILD_STATIC)
	option(LIBGS_ADD_LIBRARY_VERSION
		"-- ${PRO_NAME}: Add version information to library names." ON
	)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	option(LIBGS_USE_LIBCXX
		"-- ${PRO_NAME}: Use clang libcxx." OFF
	)
	if (LIBGS_USE_LIBCXX)
		message(STATUS "${PRO_NAME}: Use clang libcxx.")
		add_compile_options(-stdlib=libc++)
		add_link_options(-stdlib=libc++)
	endif ()

	option(LIBGS_USE_LLD
		"-- ${PRO_NAME}: Use clang lld." OFF
	)
	if (LIBGS_USE_LLD)
		message(STATUS "${PRO_NAME}: Use clang lld.")
		set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
	endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	option(LIBGS_ENABLE_LTO
		"-- ${PRO_NAME}: Use gnu-lto." OFF
	)
	if (LIBGS_ENABLE_LTO)
		message(STATUS "${PRO_NAME}: Use gnu-lto.")
		add_compile_options(-flto)
	endif ()
endif ()

option(LIBGS_OPENSSL_SUPPORT
	"-- ${PRO_NAME}: OpenSSL support." OFF
)
if (LIBGS_OPENSSL_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable OpenSSL support.")
	add_definitions(-DLIBGS_OPENSSL_SUPPORT=1)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	option(LIBGS_IO_URING_SUPPORT
		"-- ${PRO_NAME}: Linux I/O uring support." OFF
	)
	if (LIBGS_IO_URING_SUPPORT)
		message(STATUS "${PRO_NAME}: Enable Linux I/O uring support.")
	endif ()
endif ()

option(LIBGS_BUILD_CORO
	"-- ${PRO_NAME}: Build module <Coroutine>." ON
)
if (LIBGS_BUILD_CORO)
	message(STATUS "${PRO_NAME}: Build module <Coroutine>.")
endif ()

option(LIBGS_BUILD_HTTP
	"-- ${PRO_NAME}: Build module <HTTP>." ON
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
	"-- ${PRO_NAME}: Build module <WebSocket>." ON
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
	"-- ${PRO_NAME}: Build module <Utilities>." ON
)
if (LIBGS_BUILD_UTILITIES)
	if (NOT LIBGS_BUILD_CORO)
		message(FATAL_ERROR "${PRO_NAME}: <Utilities> module depends on <Coroutine> module.")
		unset(LIBGS_BUILD_UTILITIES)
	else ()
		message(STATUS "${PRO_NAME}: Build module <Utilities>.")
	endif ()
endif ()

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
	DESTINATION include/libgs/core/cxx
)
