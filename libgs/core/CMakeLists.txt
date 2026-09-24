# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.core)
add_project(${target_name} libgs.asio)

# LibGS exposes Asio error codes across module and application boundaries.  A
# header-only Asio build creates a separate error_category instance in every
# Windows DLL, so equal error values compare unequal after crossing a DLL
# boundary.  Keep one Asio implementation in gs.core and make every dependent
# target use it.
if (LIBGS_USING_BOOST_ASIO)
	set(asio_separate_compilation BOOST_ASIO_SEPARATE_COMPILATION)
	set(asio_dynamic_link BOOST_ASIO_DYN_LINK)
else ()
	set(asio_separate_compilation ASIO_SEPARATE_COMPILATION)
	set(asio_dynamic_link ASIO_DYN_LINK)
endif ()

if (LIBGS_BUILD_STATIC)
	target_compile_definitions(${target_name} PUBLIC ${asio_separate_compilation})
else ()
	# Sources inside gs.core resolve the implementation directly. Dependents
	# import the same symbols from the DLL.
	target_compile_definitions(${target_name}
		PRIVATE ${asio_separate_compilation}
		INTERFACE ${asio_dynamic_link}
	)
endif ()

if (LIBGS_OPENSSL_SUPPORT)
	if (LIBGS_OPENSSL_INSTALL_PREFIX)
		set(OPENSSL_ROOT_DIR "${LIBGS_OPENSSL_INSTALL_PREFIX}")
	endif ()
	find_package(OpenSSL REQUIRED)

	libgs_validate_msvc_imported_targets(OpenSSL
		LIBGS_OPENSSL_INSTALL_PREFIX OpenSSL::SSL OpenSSL::Crypto
	)
	target_link_libraries(${target_name} PUBLIC
		OpenSSL::SSL OpenSSL::Crypto
	)
endif ()

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread dl)
elseif (WIN32 AND (
	CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
	CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	# OpenSSL is an Asio dependency and is linked above. Keep its Windows API
	# closure after the static archive; compression remains in HTTP/WebSocket.
	target_link_libraries(${target_name} PUBLIC
		ws2_32 wsock32 crypt32 bcrypt ole32 shell32 uuid
	)
endif()

if (LIBGS_IO_URING_SUPPORT)
	find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
	find_library(LIBURING_LIBRARY NAMES uring)

	if (NOT LIBURING_INCLUDE_DIR OR NOT LIBURING_LIBRARY)
		message(FATAL_ERROR
			"${PRO_NAME}: Linux I/O uring support requires liburing."
		)
	endif ()

	if (LIBGS_USING_BOOST_ASIO)
		target_compile_definitions(${target_name} PUBLIC
			LIBGS_IO_URING_SUPPORT=1
			BOOST_ASIO_HAS_IO_URING=1
			BOOST_ASIO_DISABLE_EPOLL=1
		)
	else ()
		target_compile_definitions(${target_name} PUBLIC
			LIBGS_IO_URING_SUPPORT=1
			ASIO_HAS_IO_URING=1
			ASIO_DISABLE_EPOLL=1
		)
	endif ()
	add_library(LibGS::uring UNKNOWN IMPORTED)

	set_target_properties(LibGS::uring PROPERTIES
		IMPORTED_LOCATION "${LIBURING_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${LIBURING_INCLUDE_DIR}"
	)
	target_link_libraries(${target_name} PUBLIC LibGS::uring)
	mark_as_advanced(LIBURING_INCLUDE_DIR LIBURING_LIBRARY)
endif ()
