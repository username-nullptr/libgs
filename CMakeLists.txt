cmake_minimum_required(VERSION 3.12)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 8)
set(PATCH_VERSION 5)

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

include(utility)
check_compiler_version(CMAKE_CXX_STANDARD)

message(STATUS "")
message(STATUS "${PRO_NAME}: Using C++: " ${CMAKE_CXX_STANDARD})
message(STATUS "${PRO_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PRO_NAME}: Install prefix: " ${CMAKE_INSTALL_PREFIX})

set(output_dir ${CMAKE_BINARY_DIR}/output)

add_subdirectory(3rd_party)
include(3rd_party)

if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
	add_definitions(-DLIBGS_DEBUG)
endif()

include_directories(.)
add_subdirectory(libgs)
add_subdirectory(test)

option(LIBGS_BUILD_EXAMPLES
	"-- ${PRO_NAME}: Enable this to build the examples." OFF
)
if (LIBGS_BUILD_EXAMPLES)
	message(STATUS "${PRO_NAME}: Enable this to build the examples.")
	add_subdirectory(examples)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES libgs.h)
install(FILES libgs.h DESTINATION include/)

install(DIRECTORY libgs/ DESTINATION include/libgs/
	FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)