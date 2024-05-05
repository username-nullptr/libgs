set(target_name gs_core)

set(core_sources
	algorithm/base.cpp
	algorithm/byte_order.cpp
	algorithm/mime_type.cpp
	algorithm/sha1.cpp
	app_utls.cpp
	detail/app_utls_${OS_CPP}.cpp
	global.cpp
	args_parser.cpp
	ini.cpp
	execution.cpp
	log/scheduler.cpp
)

set(core_headers
	cxx/cplusplus.h
	cxx/attributes.h
	cxx/concept.h
	cxx/asio_concept.h
	cxx/type_traits.h
	cxx/remove_repeat.h
	cxx/function_traits.h
	cxx/utilities.h
	cxx/formatter.h
	cxx/exception.h
	cxx/flags.h
	global.h
	shared_mutex.h
	algorithm.h
	algorithm/base.h
	algorithm/byte_order.h
	algorithm/mime_type.h
	algorithm/uuid.h
	algorithm/sha1.h
	app_utls.h
	lock_free_queue.h
	log.h
	log/logger.h
	log/buffer.h
	log/scheduler.h
	log/mq.h
	log/context.h
	string_list.h
	value.h
	args_parser.h
	ini.h
	execution.h
	coroutine.h
)

set(core_detail_headers
	cxx/detail/utilities.h
	cxx/detail/exception.h
	cxx/detail/flags.h
	detail/execution.h
	algorithm/detail/base.h
	algorithm/detail/byte_order.h
	algorithm/detail/uuid.h
	detail/app_utls.h
	detail/lock_free_queue.h
	log/detail/logger.h
	log/detail/buffer.h
	log/detail/scheduler.h
	log/detail/mq.h
	log/detail/context.h
	detail/string_list.h
	detail/value.h
	detail/ini.h
	detail/coroutine.h
)

set(all_files
	${core_sources}
	${core_headers}
	${core_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

# target_link_libraries(${target_name} PUBLIC rttr_core)

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread)
elseif (WIN32)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		target_link_libraries(${target_name} PUBLIC ws2_32 wsock32)
	endif ()
endif()

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
install(TARGETS ${target_name} DESTINATION lib)