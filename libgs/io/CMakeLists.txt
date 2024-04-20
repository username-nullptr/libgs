set(target_name gs_io)

set(core_sources
	global.cpp
	types/opt_token.cpp
	types/stream_buffer.cpp
)

set(core_headers
	cxx/concept.hpp
	types/ip_types.h
	types/opt_token.h
	types/opt_condition.h
	types/stream_buffer.h
	global.h
	device_base.h
	timer.h
	stream.h
	socket.h
	tcp_socket.h
	udp_socket.h
	tcp_server.h
)

set(core_detail_headers
	types/detail/ip_types.h
	types/detail/opt_token.h
	types/detail/opt_condition.h
	detail/device_base.h
	detail/timer.h
	detail/stream.h
	detail/socket.h
	detail/tcp_socket.h
	detail/tcp_server.h
)

set(all_files
	${core_sources}
	${core_headers}
	${core_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

target_link_libraries(${target_name} PUBLIC gs_core)

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
