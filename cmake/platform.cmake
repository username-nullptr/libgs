# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

if (WIN32)
	set(OS_CPP win)
	set(IS_CPP winnt)
	set(install_dir bin)
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


function(check_compiler_version)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		if (CMAKE_CXX_COMPILER_VERSION LESS 17)
			message(FATAL_ERROR "The minimum version of 'Clang' required is 17.")
		endif ()
		add_compile_options(-Wall)

	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if (CMAKE_CXX_COMPILER_VERSION LESS 13)
			message(FATAL_ERROR "The minimum version of 'GNU' required is 13.")
		endif ()
		add_compile_options(-Wall)

	elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		if (MSVC_VERSION LESS 1930)
			message(FATAL_ERROR "The minimum version of 'MSVC' required is 1930 (VS2022).")
#		elseif (MSVC_VERSION GREATER_EQUAL 1950 AND MSVC_VERSION LESS 1960)
#			message(FATAL_ERROR "There is a bug in 1950(VS2026). It is awaiting to be fixed by MicroSoft...")
		endif ()
		add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0A00)
		add_compile_options(/W4 /wd4819 /Zc:preprocessor /bigobj)

	else()
		message(STATUS "Unknow compiler: " ${CMAKE_CXX_COMPILER_ID} " (" ${CMAKE_CXX_COMPILER_VERSION} ").")
	endif ()

	set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
#	set(CMAKE_CXX_STANDARD 23 PARENT_SCOPE)

endfunction()
