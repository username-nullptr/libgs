set(target_name gs_core)
add_project(${target_name})

target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread dl)
elseif (WIN32)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_link_libraries(${target_name} PUBLIC ws2_32 wsock32)
	endif ()
endif()