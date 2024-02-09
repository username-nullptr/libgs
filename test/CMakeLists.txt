add_executable(test main.cpp)

target_link_libraries(test PRIVATE gs_io)

set_target_properties(test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ../${output_dir_name}/bin
)
