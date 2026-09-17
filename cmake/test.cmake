# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

option(BUILD_TESTING
	"-- ${PRO_NAME}: Build tests." OFF
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
set(LIBGS_STRESS_SCALE 4 CACHE STRING
	"Work multiplier for LibGS stress tests (positive integer)."
)
set(LIBGS_FUZZ_SMOKE_RUNS 2048 CACHE STRING
	"Iterations per LibGS fuzz smoke test (positive integer)."
)

foreach(option LIBGS_STRESS_SCALE LIBGS_FUZZ_SMOKE_RUNS)
	if (NOT ${option} MATCHES "^[1-9][0-9]*$")
		message(FATAL_ERROR "${option} must be a positive integer.")
	endif ()
endforeach()
if (LIBGS_ENABLE_TEST_SANITIZERS AND LIBGS_ENABLE_TEST_TSAN)
	message(FATAL_ERROR
		"${PRO_NAME}: ASan/UBSan and TSan cannot be enabled together."
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

if ((LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN) AND LIBGS_ENABLE_LTO)
	message(FATAL_ERROR
		"${PRO_NAME}: Disable LIBGS_ENABLE_LTO for sanitizer builds."
	)
endif ()
