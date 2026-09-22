# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(LIBGS_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBGS_TEST_LAUNCHER)

# GCC's ThreadSanitizer cannot reserve its shadow-memory layout with the more
# aggressive address randomisation used by some recent Linux kernels.  Probe a
# per-process ASLR-disabled launcher instead of requiring developers to know the
# platform-specific setarch workaround.  The personality is inherited by
# interoperability peers spawned by Python as well.
if (LIBGS_ENABLE_TEST_TSAN AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_program(LIBGS_TEST_SETARCH_EXECUTABLE NAMES setarch)
	if (LIBGS_TEST_SETARCH_EXECUTABLE)
		execute_process (
			COMMAND ${LIBGS_TEST_SETARCH_EXECUTABLE}
				${CMAKE_SYSTEM_PROCESSOR} -R ${CMAKE_COMMAND} -E true
			RESULT_VARIABLE libgs_test_setarch_result
			OUTPUT_QUIET ERROR_QUIET
		)
		if (libgs_test_setarch_result EQUAL 0)
			set(LIBGS_TEST_LAUNCHER
				${LIBGS_TEST_SETARCH_EXECUTABLE} ${CMAKE_SYSTEM_PROCESSOR} -R
			)
			message(STATUS
				"${PRO_NAME}: TSan tests disable ASLR per process with setarch."
			)
		endif ()
	endif ()
	mark_as_advanced(LIBGS_TEST_SETARCH_EXECUTABLE)
endif ()


function(add_libgs_ctest test_name)
	add_test(NAME ${test_name} COMMAND ${LIBGS_TEST_LAUNCHER} ${ARGN})
endfunction()


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


function(configure_libgs_sanitized_test test_name)
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


function(configure_libgs_functional_test test_name)
	set_tests_properties(${test_name} PROPERTIES
		LABELS functional
		TIMEOUT ${LIBGS_FUNCTIONAL_TIMEOUT}
	)
	set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT
		"LIBGS_TEST_REPEAT=${LIBGS_FUNCTIONAL_REPEAT}"
		"LIBGS_TEST_SEED=${LIBGS_FUNCTIONAL_SEED}"
		"LIBGS_TEST_FAIL_FAST=1"
	)
	if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.22 AND libgs_gnu_runtime_dir)
		set_property(TEST ${test_name} APPEND PROPERTY
			ENVIRONMENT_MODIFICATION
			"PATH=path_list_prepend:${libgs_gnu_runtime_dir}"
		)
	endif ()

	configure_libgs_sanitized_test(${test_name})
endfunction()


# Functional tests keep their historical target and CTest names so existing
# per-test invocations continue to work.
function(add_libgs_functional_test name source library)
	set(test_target libgs.test.${name})
	add_libgs_test_program(${test_target} ${source} ${library})

	add_libgs_ctest(libgs.${name} $<TARGET_FILE:${test_target}> ${ARGN})
	configure_libgs_functional_test(libgs.${name})
endfunction()


# Compatibility wrapper for out-of-tree tests using the old helper name.
function(add_libgs_test name source library)
	add_libgs_functional_test(${name} ${source} ${library} ${ARGN})
endfunction()


function(add_libgs_stress_test name source library)
	set(test_target libgs.test.stress.${name})
	add_libgs_test_program(${test_target} ${source} ${library})

	target_compile_definitions(${test_target} PRIVATE
		LIBGS_STRESS_SCALE=${LIBGS_STRESS_SCALE}
	)
	add_libgs_ctest(libgs.stress.${name} $<TARGET_FILE:${test_target}>
		--repeat ${LIBGS_STRESS_REPEAT}
		--seed ${LIBGS_STRESS_SEED}
		--fail-fast
	)
	set_tests_properties(libgs.stress.${name} PROPERTIES
		LABELS stress
		RUN_SERIAL TRUE
		TIMEOUT ${LIBGS_STRESS_TIMEOUT}
	)
	configure_libgs_sanitized_test(libgs.stress.${name})
endfunction()


function(add_libgs_performance_test name source library)
	set(test_target libgs.test.performance.${name})
	add_libgs_test_program(${test_target} ${source} ${library})

	target_compile_definitions(${test_target} PRIVATE
		LIBGS_PERFORMANCE_SCALE=${LIBGS_PERFORMANCE_SCALE}
	)
	add_libgs_ctest(libgs.performance.${name} $<TARGET_FILE:${test_target}>)

	set_tests_properties(libgs.performance.${name} PROPERTIES
		LABELS performance
		RUN_SERIAL TRUE
		TIMEOUT ${LIBGS_PERFORMANCE_TIMEOUT}
	)
endfunction()


if (LIBGS_BUILD_FUZZERS)
	add_subdirectory(fuzz)
	message(STATUS
		"${PRO_NAME}: Other test suites disabled in the dedicated fuzzer build."
	)
else ()
	add_subdirectory(functional)

	if (LIBGS_BUILD_STRESS_TESTS)
		add_subdirectory(stress)
	endif ()

	if (LIBGS_BUILD_PERFORMANCE_TESTS)
		add_subdirectory(performance)
	endif ()

	if (NOT LIBGS_BUILD_STRESS_TESTS AND NOT LIBGS_BUILD_PERFORMANCE_TESTS)
		message(STATUS
			"${PRO_NAME}: Enable LIBGS_BUILD_STRESS_TESTS or "
			"LIBGS_BUILD_PERFORMANCE_TESTS for opt-in non-functional suites."
		)
	endif ()
endif ()
