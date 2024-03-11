set(target_name gs_core)

set(core_sources
	global.cpp
	algorithm/base.cpp
	algorithm/byte_order.cpp
	algorithm/mime_type.cpp
	algorithm/sha1.cpp
	app_utls.cpp
	detail/app_utls_${OS_CPP}.cpp
	log/writer.cpp
	string_list.cpp
	args_parser.cpp
	ini.cpp
	execution.cpp
)

set(core_headers
	cxx/cplusplus.hpp
	cxx/attributes.hpp
	cxx/concept.hpp
	cxx/asio_concept.hpp
	cxx/type_traits.hpp
	cxx/remove_repeat.hpp
	cxx/function_traits.hpp
	cxx/utilities.h
	cxx/formatter.h
	cxx/exception.h
	global.h
	shared_mutex.h
	flags.h
	algorithm.h
	algorithm/base.h
	algorithm/byte_order.h
	algorithm/mime_type.h
	algorithm/uuid.h
	algorithm/sha1.h
	app_utls.h
	log.h
	log/logger.h
	log/buffer.h
	log/writer.h
	log/types.h
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
	detail/global.h
	detail/flags.h
	algorithm/detail/base.h
	algorithm/detail/byte_order.h
	algorithm/detail/uuid.h
	detail/app_utls.h
	log/detail/logger.h
	log/detail/buffer.h
	log/detail/types.h
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