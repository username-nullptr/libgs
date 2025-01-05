if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	option(USE_LIBCXX "-- ${PRO_NAME}: use clang libcxx." OFF)
	if (USE_LIBCXX)
		message(STATUS "${PRO_NAME}: use clang libcxx.")
		add_compile_options(-stdlib=libc++)
		link_libraries(c++ c++abi)
	endif ()

	option(USE_LLD "-- ${PRO_NAME}: use clang lld." OFF)
	if (USE_LLD)
		message(STATUS "${PRO_NAME}: use clang lld.")
		set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
	endif ()

elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	option(ENABLE_LTO "-- ${PRO_NAME}: use gnu-lto." OFF)
	if (ENABLE_LTO)
		message(STATUS "${PRO_NAME}: use gnu-lto.")
		add_compile_options(-flto)
	endif ()
endif ()

if (WIN32)
	set(install_dir bin)
else ()
	set(install_dir lib)
endif()

function(check_compiler_version CMAKE_CXX_STANDARD)

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		if (${CMAKE_CXX_COMPILER_VERSION} LESS 17)
			message(FATAL_ERROR "The minimum version of 'Clang' required is 17.")
		endif ()
		add_compile_options(-Wall)

	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		if (${CMAKE_CXX_COMPILER_VERSION} LESS 13)
			message(FATAL_ERROR "The minimum version of 'GNU' required is 13.")
		endif ()
		add_compile_options(-Wall)

	elseif ("MSVC" MATCHES ${CMAKE_CXX_COMPILER_ID})
		if (${MSVC_VERSION} LESS 1930)
			message(FATAL_ERROR "The minimum version of 'MSVC' required is 1930 (VS2022).")
		endif ()
		add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0A00)
		add_compile_options(/W4 /wd4819)


	else()
		message(STATUS "Unknow compiler: " ${CMAKE_CXX_COMPILER_ID} " (" ${CMAKE_CXX_COMPILER_VERSION} ").")
	endif ()

	set(${CMAKE_CXX_STANDARD} 20 PARENT_SCOPE)
#	set(${CMAKE_CXX_STANDARD} 23 PARENT_SCOPE)

endfunction()
