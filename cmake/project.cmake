if (WIN32)
	set(OS_CPP win)
	set(IS_CPP winnt)
elseif (UNIX)
	if (APPLE)
		set(OS_CPP apple)
	else ()
		set(OS_CPP unix)
	endif ()
	set(IS_CPP posix)
endif()

option(LIBGS_BUILD_STATIC
	"-- ${PRO_NAME}: Build static libraries." OFF
)
if (NOT LIBGS_BUILD_STATIC)
	option(LIBGS_ADD_LIBRARY_VERSION
		"-- ${PRO_NAME}: Add version information to library names." ON
	)
endif ()

function(add_project target_name)

	file(GLOB_RECURSE ${target_name}_sources "*.cpp" "*.c" "*.ixx")
	file(GLOB_RECURSE ${target_name}_headers "*.hpp" "*.h" "*.ipp")

	set(all_files
		${${target_name}_sources}
		${${target_name}_headers}
	)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

	if (LIBGS_BUILD_STATIC)
		add_library(${target_name} STATIC ${all_files})
	else ()
		add_library(${target_name} SHARED ${all_files})
	endif ()

	if (LIBGS_ADD_LIBRARY_VERSION)
		set_target_properties(${target_name} PROPERTIES
			VERSION ${PRO_VERSION} SOVERSION ${MAJOR_VERSION}
		)
	endif ()

	target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)
	target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	if (NOT ${ARGN} STREQUAL "")
		target_link_libraries(${target_name} PUBLIC ${ARGN})
	endif ()

	set_target_properties(${target_name} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
		RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
		ARCHIVE_OUTPUT_DIRECTORY ${output_dir}/lib
	)
	install(TARGETS ${target_name} DESTINATION ${install_dir})

endfunction ()