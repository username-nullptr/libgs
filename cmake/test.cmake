# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

option(BUILD_TESTING
	"-- ${PRO_NAME}: Build tests." OFF
)
option(LIBGS_ENABLE_TEST_SANITIZERS
	"-- ${PRO_NAME}: Enable ASan and UBSan for functional tests." OFF
)
option(LIBGS_ENABLE_TEST_TSAN
	"-- ${PRO_NAME}: Enable TSan for functional tests." OFF
)
option(LIBGS_BUILD_FUZZERS
	"-- ${PRO_NAME}: Build Clang libFuzzer protocol harnesses." OFF
)
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
