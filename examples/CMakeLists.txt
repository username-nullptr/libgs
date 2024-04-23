function(set_target)
	set(INDEX 1)
	math(EXPR MAX "${ARGC}")

	set(lib_file ${ARGV0})
	set(sources)

	while (INDEX LESS ${MAX})
		set(cpp_file ${ARGV${INDEX}})
		list(APPEND sources ${cpp_file})

		string(REPLACE ".cpp" "" target_name ${cpp_file})
		string(FIND ${target_name} "/" s_index REVERSE)

		string(SUBSTRING ${target_name} 0 ${s_index} path)
		math(EXPR s_index "${s_index} + 1")

		string(SUBSTRING ${target_name} ${s_index} -1 target_name)
		add_executable(${target_name} ${cpp_file})

		target_include_directories(${target_name} PRIVATE ../../libgs)
		target_link_libraries(${target_name} PRIVATE ${lib_file})

		set_target_properties(${target_name} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${output_dir}/examples/${path}/
		)
		install(TARGETS ${target_name} DESTINATION examples/${path})
		math(EXPR INDEX "${INDEX} + 1")
	endwhile ()

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
endfunction()

set(core_sources
	core/algorithm.cpp
	core/app_utls.cpp
	core/lock_free_queue.cpp
	core/log.cpp
	core/value.cpp
	core/ini.cpp
	core/argv_parse.cpp
	core/coroutine.cpp
)
set_target(gs_core ${core_sources})

set(io_sources
	io/timer.cpp
	io/tcp_socket.cpp
	io/udp_socket.cpp
	io/tcp_server.cpp
)
set_target(gs_io ${io_sources})

set(http_sources
	http/http_server_parser.cpp
	http/http_server.cpp
)
set_target(gs_http ${http_sources})