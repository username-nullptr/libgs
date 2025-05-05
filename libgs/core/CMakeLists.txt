set(target_name gs_core)

set(${target_name}_sources
	algorithm/sha1.cpp
	app_utls.cpp
	detail/app_utls_${OS_CPP}.cpp
	global.cpp
	modules.cpp
	args_parser.cpp
	detail/ini.cpp
	execution.cpp
	mime_type.cpp
	library.cpp
	detail/library_${OS_CPP}.cpp
)

set(${target_name}_headers
	cxx/cplusplus.h
	cxx/attributes.h
	cxx/concepts.h
	cxx/string_concepts.h
	cxx/type_traits.h
	cxx/remove_repeat.h
	cxx/function_traits.h
	cxx/operators.h
	cxx/tools.h
	cxx/exception.h
	algorithm/uuid.h
	algorithm/sha1.h
	algorithm/math.h
	algorithm/misc.h
	utils/initialize.h
	utils/opt_token.h
	utils/asio_concepts.h
	utils/token_concepts.h
	utils/fstream_concepts.h
	utils/formatter.h
	utils/string_tools.h
	utils/asio_tools.h
	utils/byte_order.h
	utils/utils.h
	utils/flags.h
	global.h
	modules.h
	spin_mutex.h
	shared_mutex.h
	algorithm.h
	app_utls.h
	lock_free_queue.h
	string_container.h
	string_vector.h
	string_deque.h
	string_list.h
	string_set.h
	value.h
	args_parser.h
	ini.h
	mime_type.h
	execution.h
	library.h
)

set(${target_name}_detail_headers
	cxx/detail/operators.h
	cxx/detail/tools.h
	cxx/detail/exception.h
	algorithm/detail/uuid.h
	algorithm/detail/math.h
	algorithm/detail/misc.h
	utils/detail/opt_token.h
	utils/detail/string_tools.h
	utils/detail/asio_tools.h
	utils/detail/byte_order.h
	utils/detail/utils.h
	utils/detail/flags.h
	detail/global.h
	detail/modules.h
	detail/spin_mutex.h
	detail/shared_mutex.h
	detail/app_utls.h
	detail/lock_free_queue.h
	detail/string_container.h
	detail/value.h
	detail/ini.h
	detail/execution.h
	detail/mime_type.h
	detail/library_impl.hii
	detail/library.h
)

set(all_files
	${${target_name}_sources}
	${${target_name}_headers}
	${${target_name}_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

if (BUILD_STATIC)
	add_library(${target_name} STATIC ${all_files})
else ()
	add_library(${target_name} SHARED ${all_files})
endif ()

set_target_properties(${target_name} PROPERTIES
	VERSION ${PRO_VERSION} SOVERSION MAJOR_VERSION
)
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread dl)
elseif (WIN32)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_link_libraries(${target_name} PUBLIC ws2_32 wsock32)
	endif ()
endif()

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
	ARCHIVE_OUTPUT_DIRECTORY ${output_dir}/bin
)
install(TARGETS ${target_name} DESTINATION ${install_dir})
