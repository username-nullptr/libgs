cmake_minimum_required(VERSION 3.12)

project(LibGS VERSION 0.4.15 LANGUAGES C CXX)
add_definitions(-DLIBGS_VERSION_STR="0.4.15-dev")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(utility)
check_compiler_version(CMAKE_CXX_STANDARD)

message(STATUS "")
message(STATUS "libgs: using C++: " ${CMAKE_CXX_STANDARD})
message(STATUS "libgs: build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "libgs: install prefix: " ${CMAKE_INSTALL_PREFIX})

set(output_dir ${CMAKE_BINARY_DIR}/output)

add_subdirectory(3rd_party)
include(3rd_party)

if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
	add_definitions(-DLIBGS_DEBUG)
endif()

include_directories(.)
add_subdirectory(libgs)
add_subdirectory(test)

option(BUILD_EXAMPLES "-- libgs: enable this to build the examples" ON)

if (BUILD_EXAMPLES)
	message(STATUS "libgs: enable this to build the examples.")
	add_subdirectory(examples)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES libgs.h)
install(DIRECTORY libgs/ DESTINATION include/libgs/ FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES libgs.h DESTINATION include/)
