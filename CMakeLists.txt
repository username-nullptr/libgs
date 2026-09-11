# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)

set(MAJOR_VERSION  0)
set(MINOR_VERSION 13)
set(PATCH_VERSION  5)

set(PRO_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
set(PRO_NAME LibGS)

project(${PRO_NAME} VERSION ${PRO_VERSION} LANGUAGES C CXX)

add_definitions (
	-DLIBGS_MAJOR_VERSION=${MAJOR_VERSION}
	-DLIBGS_MINOR_VERSION=${MINOR_VERSION}
	-DLIBGS_PATCH_VERSION=${PATCH_VERSION}
	-DLIBGS_VERSION_STR="${PRO_VERSION}-dev"
)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(platform)
check_compiler_version(CMAKE_CXX_STANDARD)

message(STATUS "")
message(STATUS "${PRO_NAME}: Using C++: " ${CMAKE_CXX_STANDARD})
message(STATUS "${PRO_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PRO_NAME}: Install prefix: " ${CMAKE_INSTALL_PREFIX})

set(LIBGS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)

option(BUILD_TESTING
	"-- ${PRO_NAME}: Build tests." OFF
)
include(CTest)

option(LIBGS_ENABLE_TEST_SANITIZERS
	"-- ${PRO_NAME}: Enable ASan and UBSan for functional tests." OFF
)
option(LIBGS_ENABLE_TEST_TSAN
	"-- ${PRO_NAME}: Enable TSan for functional tests." OFF
)
if (LIBGS_ENABLE_TEST_SANITIZERS AND LIBGS_ENABLE_TEST_TSAN)
	message(FATAL_ERROR
		"${PRO_NAME}: ASan/UBSan and TSan cannot be enabled together."
	)
endif ()

if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
	if (NOT BUILD_TESTING)
		message(FATAL_ERROR
			"${PRO_NAME}: Test sanitizers require BUILD_TESTING=ON."
		)
	endif ()
	if (MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
		message(FATAL_ERROR
			"${PRO_NAME}: Test sanitizers require GCC or Clang with a GNU-style driver."
		)
	endif ()

	add_library(libgs.test.sanitizer INTERFACE)
	if (LIBGS_ENABLE_TEST_SANITIZERS)
		target_compile_options(libgs.test.sanitizer INTERFACE
			-fsanitize=address,undefined
			-fno-omit-frame-pointer
			-fno-sanitize-recover=all
		)
		target_link_options(libgs.test.sanitizer INTERFACE
			-fsanitize=address,undefined
		)
	else ()
		target_compile_options(libgs.test.sanitizer INTERFACE
			-fsanitize=thread
			-fno-omit-frame-pointer
			-fno-sanitize-recover=all
		)
		target_link_options(libgs.test.sanitizer INTERFACE -fsanitize=thread)
	endif ()
endif ()

include(options)
if ((LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN) AND LIBGS_ENABLE_LTO)
	message(FATAL_ERROR
		"${PRO_NAME}: Disable LIBGS_ENABLE_LTO for sanitizer builds."
	)
endif ()

add_subdirectory(3rd_party)
include(3rd_party)

if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
	add_definitions(-DLIBGS_DEBUG)
endif()

include_directories(.)
add_subdirectory(libgs)

if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
	foreach(target gs.core gs.http gs.websocket gs.utils)
		if (TARGET ${target})
			target_link_libraries(${target} PRIVATE libgs.test.sanitizer)
		endif ()
	endforeach()
endif ()

if (BUILD_TESTING)
	add_subdirectory(test)
endif()

option(LIBGS_BUILD_EXAMPLES
	"-- ${PRO_NAME}: Enable this to build the examples." OFF
)
if (LIBGS_BUILD_EXAMPLES)
	message(STATUS "${PRO_NAME}: Enable this to build the examples.")
	add_subdirectory(examples)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES libgs.h)
install(FILES libgs.h DESTINATION include/)


function(libgs_install_module_headers module)
	install(FILES libgs/${module}.h DESTINATION include/libgs/)
	install(DIRECTORY libgs/${module}/ DESTINATION include/libgs/${module}/
		FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.ipp"
	)
endfunction()


libgs_install_module_headers(core)

if (LIBGS_BUILD_CORO)
	libgs_install_module_headers(coro)
endif ()

if (LIBGS_BUILD_HTTP)
	libgs_install_module_headers(http)
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	libgs_install_module_headers(websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	libgs_install_module_headers(utils)
endif ()
