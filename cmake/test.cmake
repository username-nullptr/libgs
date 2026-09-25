# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

option(BUILD_TESTING
	"-- ${PRO_NAME}: Build tests." OFF
)
option(LIBGS_BUILD_CMAKE_TESTS
	"-- ${PRO_NAME}: Test CMake option constraints and the installed package."
	${BUILD_TESTING}
)
option(LIBGS_ENABLE_TEST_SANITIZERS
	"-- ${PRO_NAME}: Enable ASan and UBSan for functional and stress tests." OFF
)
option(LIBGS_ENABLE_TEST_TSAN
	"-- ${PRO_NAME}: Enable TSan for functional and stress tests." OFF
)
option(LIBGS_BUILD_FUZZERS
	"-- ${PRO_NAME}: Build Clang libFuzzer API and input harnesses." OFF
)
option(LIBGS_BUILD_STRESS_TESTS
	"-- ${PRO_NAME}: Build high-pressure stability tests." OFF
)
option(LIBGS_BUILD_PERFORMANCE_TESTS
	"-- ${PRO_NAME}: Build performance-sensitive benchmarks." OFF
)
set(LIBGS_FUNCTIONAL_REPEAT 1 CACHE STRING
	"Execution count for each LibGS functional test case (positive integer)."
)
set(LIBGS_FUNCTIONAL_SEED 1 CACHE STRING
	"Base seed for reproducible LibGS functional tests (non-negative integer)."
)
set(LIBGS_FUNCTIONAL_TIMEOUT 60 CACHE STRING
	"CTest timeout in seconds for each LibGS functional executable."
)
set(LIBGS_STRESS_SCALE 4 CACHE STRING
	"Work multiplier for LibGS stress tests (positive integer)."
)
set(LIBGS_STRESS_REPEAT 1 CACHE STRING
	"Fixture recreation count for each LibGS stress case (positive integer)."
)
set(LIBGS_STRESS_SEED 1 CACHE STRING
	"Base seed for reproducible LibGS stress scheduling (non-negative integer)."
)
set(LIBGS_STRESS_TIMEOUT 180 CACHE STRING
	"CTest timeout in seconds for each LibGS stress executable."
)
set(LIBGS_FUZZ_SMOKE_RUNS 2048 CACHE STRING
	"Iterations per LibGS fuzz smoke test (positive integer)."
)
set(LIBGS_FUZZ_SEED 1 CACHE STRING
	"Base seed for reproducible LibGS fuzz smoke tests (non-negative integer)."
)
set(LIBGS_FUZZ_MAX_LENGTH 4096 CACHE STRING
	"Maximum input size for LibGS fuzz smoke tests (positive integer)."
)
set(LIBGS_FUZZ_TIMEOUT 5 CACHE STRING
	"Per-input timeout in seconds for LibGS fuzz smoke tests."
)
set(LIBGS_FUZZ_RSS_LIMIT_MB 1024 CACHE STRING
	"Memory limit in MiB for LibGS fuzz smoke tests."
)
set(LIBGS_PERFORMANCE_SCALE 1 CACHE STRING
	"Work multiplier for LibGS performance tests (positive integer)."
)
set(LIBGS_PERFORMANCE_TIMEOUT 60 CACHE STRING
	"CTest timeout in seconds for each LibGS performance executable."
)
foreach(option
	LIBGS_FUNCTIONAL_REPEAT
	LIBGS_FUNCTIONAL_TIMEOUT
	LIBGS_STRESS_SCALE
	LIBGS_STRESS_REPEAT
	LIBGS_STRESS_TIMEOUT
	LIBGS_FUZZ_SMOKE_RUNS
	LIBGS_FUZZ_MAX_LENGTH
	LIBGS_FUZZ_TIMEOUT
	LIBGS_FUZZ_RSS_LIMIT_MB
	LIBGS_PERFORMANCE_SCALE
	LIBGS_PERFORMANCE_TIMEOUT
)
	if (NOT ${option} MATCHES "^[1-9][0-9]*$")
		message(FATAL_ERROR "${option} must be a positive integer.")
	endif ()
endforeach()

foreach(option LIBGS_FUNCTIONAL_SEED LIBGS_STRESS_SEED LIBGS_FUZZ_SEED)
	if (NOT ${option} MATCHES "^[0-9]+$")
		message(FATAL_ERROR "${option} must be a non-negative integer.")
	endif ()
endforeach()

if (LIBGS_ENABLE_TEST_SANITIZERS AND LIBGS_ENABLE_TEST_TSAN)
	message(FATAL_ERROR
		"${PRO_NAME}: ASan/UBSan and TSan cannot be enabled together."
	)
endif ()

if (LIBGS_BUILD_CMAKE_TESTS AND NOT BUILD_TESTING)
	message(FATAL_ERROR
		"${PRO_NAME}: CMake integration tests require BUILD_TESTING=ON."
	)
endif ()

if ((LIBGS_BUILD_STRESS_TESTS OR LIBGS_BUILD_PERFORMANCE_TESTS) AND NOT BUILD_TESTING)
	message(FATAL_ERROR
		"${PRO_NAME}: Stress and performance tests require BUILD_TESTING=ON."
	)
endif ()

if (LIBGS_BUILD_PERFORMANCE_TESTS AND
	(LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN))
	message(FATAL_ERROR
		"${PRO_NAME}: Performance tests cannot be combined with test sanitizers."
	)
endif ()

if (LIBGS_BUILD_FUZZERS)
	if (NOT BUILD_TESTING)
		message(FATAL_ERROR "${PRO_NAME}: Fuzzers require BUILD_TESTING=ON.")
	endif ()

	if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		message(FATAL_ERROR "${PRO_NAME}: Fuzzers require Clang libFuzzer.")
	endif ()

	if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
		message(FATAL_ERROR
			"${PRO_NAME}: Fuzzer instrumentation cannot be combined with test sanitizer options."
		)
	endif ()

	if (LIBGS_BUILD_STRESS_TESTS OR LIBGS_BUILD_PERFORMANCE_TESTS)
		message(FATAL_ERROR
			"${PRO_NAME}: Fuzzers use a dedicated build; disable stress and performance tests."
		)
	endif ()

	if (LIBGS_BUILD_EXAMPLES)
		message(FATAL_ERROR
			"${PRO_NAME}: Fuzzers use a dedicated build; disable examples."
		)
	endif ()

	include(CheckCXXSourceCompiles)
	set(libgs_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
	set(libgs_saved_required_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")
	set(libgs_saved_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
	set(libgs_fuzzer_source
		"#include <cstddef>\n#include <cstdint>\nextern \"C\" int LLVMFuzzerTestOneInput(const uint8_t*, size_t) { return 0; }"
	)
	set(libgs_fuzzer_compile_flags
		-fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer
	)
	set(libgs_fuzzer_link_options -fsanitize=fuzzer,address,undefined)
	if (LIBGS_USE_LIBCXX)
		list(APPEND libgs_fuzzer_compile_flags -stdlib=libc++)
		list(APPEND libgs_fuzzer_link_options -stdlib=libc++)
	endif ()
	if (LIBGS_USE_LLD)
		list(APPEND libgs_fuzzer_link_options -fuse-ld=lld)
	endif ()
	string(JOIN " " libgs_fuzzer_compile_flags_string
		${libgs_fuzzer_compile_flags}
	)

	set(CMAKE_REQUIRED_FLAGS
		"${libgs_saved_required_flags} ${libgs_fuzzer_compile_flags_string}"
	)
	set(CMAKE_REQUIRED_LINK_OPTIONS
		${libgs_saved_required_link_options}
		${libgs_fuzzer_link_options}
	)
	unset(LIBGS_FUZZER_INSTRUMENTATION_AVAILABLE CACHE)

	check_cxx_source_compiles(
		"${libgs_fuzzer_source}"
		LIBGS_FUZZER_INSTRUMENTATION_AVAILABLE
	)

	# Some Linux compiler-rt packages build libFuzzer against libstdc++ even
	# when Clang and libc++ are installed together. In that case the driver can
	# compile the probe but cannot link the fuzzer runtime with libc++. Place the
	# packaged runtime after ASan and satisfy only its private ABI dependency.
	unset(LIBGS_FUZZER_COMPAT_RUNTIME_AVAILABLE CACHE)
	unset(LIBGS_FUZZER_RUNTIME_LIBRARY)
	if (NOT LIBGS_FUZZER_INSTRUMENTATION_AVAILABLE AND
		LIBGS_USE_LIBCXX AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
		execute_process(
			COMMAND ${CMAKE_CXX_COMPILER} --print-runtime-dir
			OUTPUT_VARIABLE libgs_clang_runtime_dir
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE libgs_clang_runtime_dir_result
			ERROR_QUIET
		)
		if (CMAKE_CXX_COMPILER_TARGET)
			set(libgs_clang_target "${CMAKE_CXX_COMPILER_TARGET}")
		else ()
			execute_process(
				COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
				OUTPUT_VARIABLE libgs_clang_target
				OUTPUT_STRIP_TRAILING_WHITESPACE
				RESULT_VARIABLE libgs_clang_target_result
				ERROR_QUIET
			)
		endif ()
		string(REGEX MATCH "^[^-]+" libgs_clang_runtime_arch
			"${libgs_clang_target}"
		)
		set(libgs_fuzzer_runtime_candidate
			"${libgs_clang_runtime_dir}/libclang_rt.fuzzer-${libgs_clang_runtime_arch}.a"
		)

		if (libgs_clang_runtime_dir_result EQUAL 0 AND
			EXISTS "${libgs_fuzzer_runtime_candidate}")
			set(libgs_fuzzer_compat_compile_flags
				-fsanitize=fuzzer-no-link,address,undefined
				-fno-omit-frame-pointer
				-stdlib=libc++
			)
			string(JOIN " " libgs_fuzzer_compat_compile_flags_string
				${libgs_fuzzer_compat_compile_flags}
			)
			set(CMAKE_REQUIRED_FLAGS
				"${libgs_saved_required_flags} ${libgs_fuzzer_compat_compile_flags_string}"
			)
			set(CMAKE_REQUIRED_LINK_OPTIONS
				${libgs_saved_required_link_options}
				-fsanitize=fuzzer-no-link,address,undefined
				-stdlib=libc++
			)
			if (LIBGS_USE_LLD)
				list(APPEND CMAKE_REQUIRED_LINK_OPTIONS -fuse-ld=lld)
			endif ()
			set(CMAKE_REQUIRED_LIBRARIES
				"${libgs_fuzzer_runtime_candidate}" -Wl,-lstdc++
			)
			check_cxx_source_compiles(
				"${libgs_fuzzer_source}"
				LIBGS_FUZZER_COMPAT_RUNTIME_AVAILABLE
			)
			if (LIBGS_FUZZER_COMPAT_RUNTIME_AVAILABLE)
				set(LIBGS_FUZZER_RUNTIME_LIBRARY
					"${libgs_fuzzer_runtime_candidate}"
				)
				set(LIBGS_FUZZER_INSTRUMENTATION_AVAILABLE TRUE)
				message(STATUS
					"${PRO_NAME}: Use the libstdc++-built libFuzzer runtime with libc++."
				)
			endif ()
		endif ()
	endif ()
	set(CMAKE_REQUIRED_FLAGS "${libgs_saved_required_flags}")
	set(CMAKE_REQUIRED_LINK_OPTIONS ${libgs_saved_required_link_options})
	set(CMAKE_REQUIRED_LIBRARIES ${libgs_saved_required_libraries})

	if (NOT LIBGS_FUZZER_INSTRUMENTATION_AVAILABLE)
		message(FATAL_ERROR
			"${PRO_NAME}: Clang libFuzzer is unavailable for the selected C++ runtime."
		)
	endif ()
endif ()

if ((LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN) AND LIBGS_ENABLE_LTO)
	message(FATAL_ERROR
		"${PRO_NAME}: Disable LIBGS_ENABLE_LTO for sanitizer builds."
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

	include(CheckCXXSourceCompiles)
	set(libgs_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
	set(libgs_saved_required_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")

	if (LIBGS_ENABLE_TEST_SANITIZERS)
		set(libgs_sanitizer_flags -fsanitize=address,undefined)
	else ()
		set(libgs_sanitizer_flags -fsanitize=thread)
	endif ()

	set(CMAKE_REQUIRED_FLAGS
		"${libgs_saved_required_flags} ${libgs_sanitizer_flags}"
	)
	set(CMAKE_REQUIRED_LINK_OPTIONS
		${libgs_saved_required_link_options} ${libgs_sanitizer_flags}
	)
	unset(LIBGS_TEST_SANITIZER_AVAILABLE CACHE)

	check_cxx_source_compiles (
		"int main() { return 0; }"
		LIBGS_TEST_SANITIZER_AVAILABLE
	)
	set(CMAKE_REQUIRED_FLAGS "${libgs_saved_required_flags}")
	set(CMAKE_REQUIRED_LINK_OPTIONS ${libgs_saved_required_link_options})

	if (NOT LIBGS_TEST_SANITIZER_AVAILABLE)
		message(FATAL_ERROR
			"${PRO_NAME}: Requested test sanitizer runtime is unavailable."
		)
	endif ()

	add_library(libgs.test.sanitizer INTERFACE)
	set_target_properties(libgs.test.sanitizer PROPERTIES
		EXPORT_NAME sanitizer
	)
	add_library(LibGS::sanitizer ALIAS libgs.test.sanitizer)
	install(TARGETS libgs.test.sanitizer EXPORT LibGSTargets)

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
