# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.core)
add_project(${target_name})

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread dl)
elseif (WIN32)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_link_libraries(${target_name} PUBLIC ws2_32 wsock32)
	endif ()
endif()

if (LIBGS_OPENSSL_SUPPORT)
	find_package(OpenSSL REQUIRED)
	target_link_libraries(${target_name} PUBLIC
		OpenSSL::SSL OpenSSL::Crypto
	)
endif ()

if (LIBGS_IO_URING_SUPPORT)
	find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
	find_library(LIBURING_LIBRARY NAMES uring)

	if (NOT LIBURING_INCLUDE_DIR OR NOT LIBURING_LIBRARY)
		message(FATAL_ERROR
			"${PRO_NAME}: Linux I/O uring support requires liburing."
		)
	endif ()

	target_compile_definitions(${target_name} PUBLIC
		LIBGS_IO_URING_SUPPORT=1
		ASIO_HAS_IO_URING=1
		ASIO_DISABLE_EPOLL=1
	)
	target_include_directories(${target_name} PUBLIC ${LIBURING_INCLUDE_DIR})
	target_link_libraries(${target_name} PUBLIC ${LIBURING_LIBRARY})
	mark_as_advanced(LIBURING_INCLUDE_DIR LIBURING_LIBRARY)
endif ()
