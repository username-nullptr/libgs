# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

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
		if (LIBGS_ADD_LIBRARY_VERSION)
			set_target_properties(${target_name} PROPERTIES
				VERSION ${PRO_VERSION} SOVERSION ${MAJOR_VERSION}
			)
		endif ()
	endif ()

	string(REPLACE "." "_" target_micro "${target_name}")
	target_compile_definitions(${target_name} PRIVATE ${target_micro}_EXPORTS)
#	target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	if (NOT ${ARGN} STREQUAL "")
		target_link_libraries(${target_name} PUBLIC ${ARGN})
	endif ()

	set_target_properties(${target_name} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
		ARCHIVE_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/lib
	)
	if (LIBGS_BUILD_STATIC)
		install(TARGETS ${target_name} ARCHIVE DESTINATION lib)
	else ()
		install(TARGETS ${target_name} DESTINATION ${install_dir}
			PERMISSIONS
			OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE
		)
	endif ()

endfunction ()
