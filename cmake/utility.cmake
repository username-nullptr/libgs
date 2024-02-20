if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

	option(USE_LIBCXX "-- libgs: use clang libcxx." OFF)
	if (USE_LIBCXX)
		message(STATUS "libgs: use clang libcxx.")
		add_compile_options(-stdlib=libc++)
		link_libraries(c++ c++abi)
	endif ()

	option(USE_LLD "-- libgs: use clang lld." OFF)
	if (USE_LLD)
		message(STATUS "libgs: use clang lld.")
		set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
	endif ()

endif ()

function(check_compiler_version CMAKE_CXX_STANDARD)

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		if (${CMAKE_CXX_COMPILER_VERSION} LESS 17)
			message(FATAL_ERROR "The minimum version of 'Clang' required is 17.")
		endif ()

	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		if (${CMAKE_CXX_COMPILER_VERSION} LESS 17)
			message(FATAL_ERROR "The minimum version of 'GNU' required is 13.")
		endif ()

	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
		if (${MSVC_VERSION} LESS 1930)
			message(FATAL_ERROR "The minimum version of 'MSVC' required is 1930 (VS2022).")
		endif ()

	else()
		message(STATUS "Unknow compiler: " ${CMAKE_CXX_COMPILER_ID} " (" ${CMAKE_CXX_COMPILER_VERSION} ").")
	endif ()

	set(${CMAKE_CXX_STANDARD} 20 PARENT_SCOPE)
	# set(${CMAKE_CXX_STANDARD} 23 PARENT_SCOPE)

endfunction()
