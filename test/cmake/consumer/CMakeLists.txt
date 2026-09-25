# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)
project(libgs_install_consumer LANGUAGES CXX)

if (LIBGS_REQUEST_MISSING_COMPONENT)
	find_package(LibGS CONFIG QUIET COMPONENTS
		${LIBGS_REQUEST_MISSING_COMPONENT}
	)
	if (LibGS_FOUND)
		message(FATAL_ERROR
			"LibGS unexpectedly provides component "
			"'${LIBGS_REQUEST_MISSING_COMPONENT}'."
		)
	endif ()

	message(STATUS
		"LibGS correctly rejected component '${LIBGS_REQUEST_MISSING_COMPONENT}'."
	)
	return()
endif ()

if (NOT LIBGS_EXPECTED_COMPONENTS)
	message(FATAL_ERROR "LIBGS_EXPECTED_COMPONENTS is required.")
endif ()

string(REPLACE "," ";" libgs_components "${LIBGS_EXPECTED_COMPONENTS}")

find_package(LibGS ${LIBGS_EXPECTED_VERSION} EXACT CONFIG REQUIRED
	COMPONENTS ${libgs_components}
)
enable_testing()

set(libgs_legacy_targets)
set(libgs_expected_definitions)

if (WIN32 AND LIBGS_RUNTIME_DIR)
	file(GLOB libgs_runtime_dlls "${LIBGS_RUNTIME_DIR}/*.dll")
endif ()

function(add_libgs_consumer_runtime_test target)
	# Keep compatibility with CMake 3.15, which predates TARGET_RUNTIME_DLLS and
	# CTest's ENVIRONMENT_MODIFICATION property.
	if (libgs_runtime_dlls)
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${libgs_runtime_dlls} "$<TARGET_FILE_DIR:${target}>"
			COMMAND_EXPAND_LISTS
		)
	endif ()

	add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction()

foreach(component IN LISTS libgs_components)
	if (NOT LibGS_${component}_FOUND)
		message(FATAL_ERROR "Component '${component}' was not reported as found.")
	endif ()

	if (NOT TARGET LibGS::${component})
		message(FATAL_ERROR "Missing imported target LibGS::${component}.")
	endif ()

	if (NOT TARGET gs.${component})
		message(FATAL_ERROR "Missing compatibility target gs.${component}.")
	endif ()

	string(TOUPPER "${component}" component_upper)
	set(target libgs_consumer_${component})

	add_executable(${target} main.cpp)
	target_compile_features(${target} PRIVATE cxx_std_20)

	target_compile_definitions(${target} PRIVATE
		"LIBGS_CONSUME_${component_upper}=1"
	)
	target_link_libraries(${target} PRIVATE LibGS::${component})
	add_libgs_consumer_runtime_test(${target})

	list(APPEND libgs_legacy_targets gs.${component})
	list(APPEND libgs_expected_definitions
		"LIBGS_CONSUME_${component_upper}=1"
	)
endforeach()

add_executable(libgs_consumer_legacy main.cpp)
target_compile_features(libgs_consumer_legacy PRIVATE cxx_std_20)

target_compile_definitions(libgs_consumer_legacy PRIVATE
	${libgs_expected_definitions}
	LIBGS_CONSUME_UMBRELLA=1
)
target_link_libraries(libgs_consumer_legacy PRIVATE ${libgs_legacy_targets})
add_libgs_consumer_runtime_test(libgs_consumer_legacy)
