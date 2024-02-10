set(target_name gs_io)

set(core_sources
	detail/waitable_${IS_CPP}.cpp
	device.cpp
	socket.cpp
	tcp_socket.cpp
	tcp_ssl_socket.cpp
)

set(core_headers
	global.h
	timer.h
	device.h
	socket.h
	tcp_socket.h
	tcp_ssl_socket.h
)

set(core_detail_headers
	detail/timer.h
	detail/waitable.h
	detail/device.h
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
