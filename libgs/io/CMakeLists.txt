set(target_name gs_io)

set(core_sources
	global.cpp
)

set(core_headers
	cxx/concept.hpp
	global.h
	device_base.h
	timer.h
	stream_buffer.h
	stream.h
	socket.h
	tcp_socket.h
)

set(core_detail_headers
	detail/stream_buffer.h
	detail/device_base.h
	detail/stream.h
	detail/socket.h
	detail/tcp_socket.h
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
	LIBRARY_OUTPUT_DIRECTORY ../../${output_dir_name}/bin
)
