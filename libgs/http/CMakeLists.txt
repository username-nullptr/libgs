set(target_name gs_http)

set(http_sources
	global.cpp
	types.cpp
)

set(http_headers
	global.h
	container.h
	cookie.h
	session.h
	types.h
	request.h
	response.h
	parser.h
)

set(http_detail_headers
	detail/container.h
	detail/cookie.h
)

set(all_files
	${http_sources}
	${http_headers}
	${http_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

target_link_libraries(${target_name} PUBLIC gs_core)

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
