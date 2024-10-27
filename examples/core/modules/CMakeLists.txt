set(target_name modules)

file(GLOB_RECURSE ${target_name}_sources "*.cpp" "*.c")
file(GLOB_RECURSE ${target_name}_headers "*.hpp" "*.h")

set(all_files
	${${target_name}_sources}
	${${target_name}_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_executable(${target_name} ${all_files})
target_link_libraries(${target_name} PUBLIC gs_core)

set_target_properties(${target_name} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/examples/core/
)
install(TARGETS ${target_name} DESTINATION examples/core)
