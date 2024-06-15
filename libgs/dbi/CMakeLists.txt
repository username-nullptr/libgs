set(target_name gs_dbi)

set(${target_name}_sources
	management.cpp
)

set(${target_name}_headers
	global.h
	cell.h
	types.h
	management.h
)

set(${target_name}_detail_headers
	detail/cell.h
)

set(all_files
	${${target_name}_sources}
	${${target_name}_headers}
	${${target_name}_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

target_link_libraries(${target_name} PUBLIC gs_core)

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
install(TARGETS ${target_name} DESTINATION lib)
