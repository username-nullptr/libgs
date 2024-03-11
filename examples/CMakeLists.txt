function(set_target)
	set(INDEX 1)
	math(EXPR MAX "${ARGC}")

	set(lib_file ${ARGV0})
	set(sources)

	while (INDEX LESS ${MAX})
		set(cpp_file ${ARGV${INDEX}})
		list(APPEND sources ${cpp_file})

		string(REPLACE ".cpp" "" target_name ${cpp_file})
		add_executable(${target_name} ${cpp_file})

		target_include_directories(${target_name} PRIVATE ../../libgs)
		target_link_libraries(${target_name} PRIVATE ${lib_file})

		set_target_properties(${target_name} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${output_dir}/examples
		)
		math(EXPR INDEX "${INDEX} + 1")
	endwhile ()

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
endfunction()

set(core_sources
	algorithm.cpp
	app_utls.cpp
	value.cpp
	ini.cpp
	argv_parse.cpp
	coroutine.cpp
)
set_target(gs_core ${core_sources})

set(io_sources
	timer.cpp
	tcp_socket.cpp
	udp_socket.cpp
	tcp_server.cpp
)
set_target(gs_io ${io_sources})

# set(http_sources ...)
# set_target(gs_http ${http_sources})