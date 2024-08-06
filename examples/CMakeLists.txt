if (CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag("-Wa,-mbig-obj" GNU_BIG_OBJ_FLAG_ENABLE)
	message(STATUS GNU_BIG_OBJ_FLAG_ENABLE=${GNU_BIG_OBJ_FLAG_ENABLE})
endif ()

function(set_target)
	set(INDEX 1)
	math(EXPR MAX "${ARGC}")

	set(lib_file ${ARGV0})
	set(defines)
	set(sources)

	while (INDEX LESS ${MAX})
		set(cpp_file ${ARGV${INDEX}})
		list(APPEND sources ${cpp_file})

		string(REPLACE ".cpp" "" target_name ${cpp_file})
		string(FIND ${target_name} "/" s_index REVERSE)

		string(SUBSTRING ${target_name} 0 ${s_index} path)
		math(EXPR s_index "${s_index} + 1")

		string(SUBSTRING ${target_name} ${s_index} -1 target_name)
		set(headers ../../libgs)
		set(lib_path)

		if (${target_name} MATCHES ssl OR ${target_name} MATCHES https)
			if (NOT OpenSSL_DIR)
				math(EXPR INDEX "${INDEX} + 1")
				continue ()
			endif()

			list(APPEND defines LIBGS_ENABLE_OPENSSL)
			list(APPEND headers ${OpenSSL_DIR}/include)
			list(APPEND lib_path ${OpenSSL_DIR}/lib)
			list(APPEND lib_file crypto ssl)
		endif ()
		add_executable(${target_name} ${cpp_file})

		target_compile_definitions(${target_name} PRIVATE ${defines})
		target_include_directories(${target_name} PRIVATE ${headers})

		target_link_directories(${target_name} PRIVATE ${lib_path})
		target_link_libraries(${target_name} PRIVATE ${lib_file})

		target_compile_options(${target_name} PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
			$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>
		)
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
	core/value.cpp
	core/ini.cpp
	core/argv_parse.cpp
	core/coroutine.cpp
	core/library.cpp
)
set_target(gs_core ${core_sources})

set(http_sources
	http/http_server_parser.cpp
	http/http_server.cpp
	http/http_server_aop.cpp
	http/http_session.cpp
	http/https_server.cpp
)
set_target(gs_core ${http_sources})