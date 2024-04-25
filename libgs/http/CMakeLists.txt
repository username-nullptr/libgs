set(target_name gs_http)

set(http_sources
	global.cpp
	parser.cpp
)

set(http_headers
	global.h
	parser.h
	client.h
	server.h
	basic/container.h
	basic/cookie.h
	basic/header.h
	basic/types.h
	basic/opt_token.h
	client/request_helper.h
	client/request.h
	server/request.h
	server/request_parser.h
	server/response_helper.h
	server/response.h
	server/session.h
	server/server.h
)

set(http_detail_headers
	basic/detail/container.h
	basic/detail/cookie.h
	basic/detail/types.h
	basic/detail/opt_token.h
	server/detail/request_parser.h
	server/detail/request.h
	server/detail/response_helper.h
	server/detail/response.h
	server/detail/server.h
)

set(all_files
	${http_sources}
	${http_headers}
	${http_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

target_link_libraries(${target_name} PUBLIC gs_io)

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
install(TARGETS ${target_name} DESTINATION lib)