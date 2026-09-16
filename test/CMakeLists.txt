# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(LIBGS_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag("-Wa,-mbig-obj" LIBGS_TEST_GNU_BIG_OBJ)
endif()


# Build a test program without necessarily registering it as a standalone
# CTest test. Interoperability runners use this for the LibGS peer process.
function(add_libgs_test_program test_target source library)
	add_executable(${test_target} ${source})
	target_link_libraries(${test_target} PRIVATE ${library})
	if ("${library}" MATCHES "^gs\\.(http|websocket)$")
		libgs_limit_heavy_compile(${test_target})
	endif ()
	target_compile_options(${test_target} PRIVATE
		$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${LIBGS_TEST_GNU_BIG_OBJ}>>:-Wa,-mbig-obj>
	)

	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		target_link_libraries(${test_target} PRIVATE libgs.test.sanitizer)
	endif ()
	target_include_directories(${test_target} PRIVATE ${LIBGS_TEST_SOURCE_DIR})

	set_target_properties(${test_target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	)
endfunction()


function(configure_libgs_functional_test test_name)
	set_tests_properties(${test_name} PROPERTIES
		LABELS functional
		FIXTURES_SETUP libgs.functional
		TIMEOUT 60
	)
	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		set_property(TEST ${test_name} APPEND PROPERTY LABELS sanitizer)
		set_property(TEST ${test_name} PROPERTY RUN_SERIAL TRUE)

		set_property(TEST ${test_name} APPEND PROPERTY FAIL_REGULAR_EXPRESSION
			"AddressSanitizer" "LeakSanitizer" "ThreadSanitizer" "runtime error:"
		)
		if (LIBGS_ENABLE_TEST_SANITIZERS)
			set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT
				"ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:abort_on_error=1"
				"UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
			)
		else ()
			set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT
				"TSAN_OPTIONS=halt_on_error=1"
			)
		endif ()
	endif ()
endfunction()


# Existing tests form the functional suite. Keeping their target and CTest
# names stable avoids breaking commands that select a test by name.
function(add_libgs_test name source library)
	set(test_target libgs.test.${name})
	add_libgs_test_program(${test_target} ${source} ${library})
	add_test(NAME libgs.${name} COMMAND $<TARGET_FILE:${test_target}> ${ARGN})
	configure_libgs_functional_test(libgs.${name})
endfunction()


function(add_libgs_performance_test name source library)
	set(test_target libgs.test.performance.${name})
	add_executable(${test_target} ${source})
	if ("${library}" MATCHES "^gs\\.(http|websocket)$")
		libgs_limit_heavy_compile(${test_target})
	endif ()
	target_compile_options(${test_target} PRIVATE
		$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${LIBGS_TEST_GNU_BIG_OBJ}>>:-Wa,-mbig-obj>
	)

	target_link_libraries(${test_target} PRIVATE ${library})
	target_include_directories(${test_target} PRIVATE ${LIBGS_TEST_SOURCE_DIR})

	set_target_properties(${test_target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	)
	add_test(NAME libgs.performance.${name} COMMAND $<TARGET_FILE:${test_target}>)

	set_tests_properties(libgs.performance.${name} PROPERTIES
		LABELS performance
		FIXTURES_REQUIRED libgs.functional
		RUN_SERIAL TRUE
		TIMEOUT 60
	)
endfunction()


if (LIBGS_BUILD_FUZZERS)
	add_subdirectory(fuzz)
	message(STATUS
		"${PRO_NAME}: Functional and performance tests disabled in fuzzer builds."
	)
else ()
	add_subdirectory(functional)
	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		message(STATUS "${PRO_NAME}: Performance tests disabled in sanitizer builds.")
	else ()
		add_subdirectory(performance)
	endif ()
endif ()
