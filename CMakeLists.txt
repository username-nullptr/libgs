# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 1)
set(PATCH_VERSION 7)

set(PRO_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
set(PRO_NAME LibGS)

project(${PRO_NAME} VERSION ${PRO_VERSION} LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_definitions (
	-DLIBGS_MAJOR_VERSION=${MAJOR_VERSION}
	-DLIBGS_MINOR_VERSION=${MINOR_VERSION}
	-DLIBGS_PATCH_VERSION=${PATCH_VERSION}
	-DLIBGS_VERSION_STR="${PRO_VERSION}-dev"
)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(platform)
include(heavy_compile)
include(options)
include(test)
include(CTest)

add_subdirectory(3rd_party)
include(3rd_party)

if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
	add_definitions(-DLIBGS_DEBUG)
endif()

include_directories(.)
add_subdirectory(libgs)

set(LIBGS_INSTALL_CMAKEDIR
	"${CMAKE_INSTALL_LIBDIR}/cmake/${PRO_NAME}"
	CACHE STRING "Install path for ${PRO_NAME} CMake package files."
)
set(LIBGS_PACKAGE_NEEDS_ZLIB OFF)

if (LIBGS_BUILD_STATIC AND (
	(LIBGS_BUILD_HTTP AND LIBGS_HTTP_ZLIB_SUPPORT) OR
	(LIBGS_BUILD_WEBSOCKET AND LIBGS_WEBSOCKET_ZLIB_SUPPORT) ))
	set(LIBGS_PACKAGE_NEEDS_ZLIB ON)
endif ()

configure_package_config_file (
	cmake/LibGSConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/LibGSConfig.cmake
	INSTALL_DESTINATION ${LIBGS_INSTALL_CMAKEDIR}
)
write_basic_package_version_file (
	${CMAKE_CURRENT_BINARY_DIR}/LibGSConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMinorVersion
)
install(EXPORT LibGSTargets
	FILE LibGSTargets.cmake
	NAMESPACE LibGS::
	DESTINATION ${LIBGS_INSTALL_CMAKEDIR}
)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/LibGSConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/LibGSConfigVersion.cmake
	DESTINATION ${LIBGS_INSTALL_CMAKEDIR}
)
if (BUILD_TESTING)
	add_subdirectory(test)
endif()

if (LIBGS_BUILD_EXAMPLES)
	message(STATUS "${PRO_NAME}: Enable this to build the examples.")
	add_subdirectory(examples)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES libgs.h)
install(FILES libgs.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
