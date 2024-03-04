set(example_sources
	algorithm.cpp
	value.cpp
	ini.cpp
	argv_parse.cpp
	coroutine.cpp
	timer.cpp
	tcp_socket.cpp
	udp_socket.cpp
	tcp_server.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${example_sources})

foreach(cpp_file ${example_sources})
	string(REPLACE ".cpp" "" target_name ${cpp_file})
	add_executable(${target_name} ${cpp_file})

	target_include_directories(${target_name} PRIVATE ../../libgs)
	target_link_libraries(${target_name} PRIVATE gs_io)

	set_target_properties(${target_name} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ../${output_dir_name}/examples
	)
endforeach()
