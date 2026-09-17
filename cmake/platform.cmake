# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if (CMAKE_CXX_COMPILER_VERSION LESS 17)
		message(FATAL_ERROR "The minimum version of 'Clang' required is 17.")
	endif ()
	add_compile_options(-Wall)

	option(LIBGS_USE_LIBCXX
		"-- ${PRO_NAME}: Use clang libcxx." OFF
	)
	if (LIBGS_USE_LIBCXX)
		message(STATUS "${PRO_NAME}: Use clang libcxx.")
		add_compile_options(-stdlib=libc++)
		add_link_options(-stdlib=libc++)
	endif ()

	option(LIBGS_USE_LLD
		"-- ${PRO_NAME}: Use clang lld." OFF
	)
	if (LIBGS_USE_LLD)
		message(STATUS "${PRO_NAME}: Use clang lld.")
		set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
	endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if (CMAKE_CXX_COMPILER_VERSION LESS 13)
		message(FATAL_ERROR "The minimum version of 'GNU' required is 13.")
	endif ()
	add_compile_options(-Wall)

	option(LIBGS_ENABLE_LTO
		"-- ${PRO_NAME}: Use gnu-lto." OFF
	)
	if (LIBGS_ENABLE_LTO)
		message(STATUS "${PRO_NAME}: Use gnu-lto.")
		add_compile_options(-flto)
	endif ()

elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	if (MSVC_VERSION LESS 1930)
		message(FATAL_ERROR "The minimum version of 'MSVC' required is 1930 (VS2022).")
	endif ()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0A00)
	add_compile_options(/W4 /wd4819 /Zc:preprocessor /bigobj)

else()
	message(STATUS "Unknow compiler: " ${CMAKE_CXX_COMPILER_ID} " (" ${CMAKE_CXX_COMPILER_VERSION} ").")
endif ()

set(libgs_build_static_default OFF)
set(libgs_gnu_shared_runtime_available TRUE)

if (WIN32)
	set(OS_CPP win)
	set(IS_CPP winnt)
	set(install_dir bin)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		execute_process (
			COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++-6.dll
			OUTPUT_VARIABLE libgs_gnu_libstdcxx_dll
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
		if (NOT IS_ABSOLUTE "${libgs_gnu_libstdcxx_dll}" OR NOT EXISTS "${libgs_gnu_libstdcxx_dll}")
			set(libgs_gnu_shared_runtime_available FALSE)
			set(libgs_build_static_default ON)

			message(STATUS
				"${PRO_NAME}: GNU C++ runtime is static-only."
			)
		endif ()
	endif ()

else ()
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-rdynamic)
	endif ()

	if (APPLE)
		set(OS_CPP apple)
	elseif (ANDROID)
		set(OS_CPP android)
	else ()
		set(OS_CPP unix)
	endif ()

	set(IS_CPP posix)
	set(install_dir lib)
endif()

set(CMAKE_CXX_STANDARD 20)
set(LIBGS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)

message(STATUS "")
message(STATUS "${PRO_NAME}: Using C++: " ${CMAKE_CXX_STANDARD})
message(STATUS "${PRO_NAME}: Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "${PRO_NAME}: Install prefix: " ${CMAKE_INSTALL_PREFIX})
