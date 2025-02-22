set(target_name gs_core)

set(${target_name}_sources
	algorithm/byte_order.cpp
	algorithm/mime_type.cpp
	algorithm/sha1.cpp
	algorithm/misc.cpp
	app_utls.cpp
	detail/app_utls_${OS_CPP}.cpp
	global.cpp
	modules.cpp
	args_parser.cpp
	detail/ini.cpp
	execution.cpp
	library.cpp
	detail/library_${OS_CPP}.cpp
)

set(${target_name}_headers
	cxx/cplusplus.h
	cxx/attributes.h
	cxx/concepts.h
	cxx/asio_concepts.h
	cxx/type_traits.h
	cxx/remove_repeat.h
	cxx/function_traits.h
	cxx/operators.h
	cxx/return_tools.h
	cxx/string_tools.h
	cxx/tools.h
	cxx/initialize.h
	cxx/utilities.h
	cxx/formatter.h
	cxx/exception.h
	cxx/flags.h
	global.h
	modules.h
	spin_mutex.h
	shared_mutex.h
	algorithm.h
	algorithm/base.h
	algorithm/byte_order.h
	algorithm/mime_type.h
	algorithm/uuid.h
	algorithm/sha1.h
	algorithm/math.h
	algorithm/misc.h
	coro/utilities.h
	coro/mutex.h
	coro/condition_variable.h
	coro/shared_mutex.h
	coro/semaphore.h
	app_utls.h
	lock_free_queue.h
	string_list.h
	value.h
	args_parser.h
	ini.h
	execution.h
	coro.h
	library.h
)

set(${target_name}_detail_headers
	cxx/detail/operators.h
	cxx/detail/return_tools.h
	cxx/detail/string_tools.h
	cxx/detail/tools.h
	cxx/detail/utilities.h
	cxx/detail/exception.h
	cxx/detail/flags.h
	detail/global.h
	detail/modules.h
	detail/spin_mutex.h
	detail/shared_mutex.h
	algorithm/detail/base.h
	algorithm/detail/byte_order.h
	algorithm/detail/uuid.h
	algorithm/detail/math.h
	coro/detail/utilities.h
	coro/detail/wake_up.h
	coro/detail/mutex.h
	coro/detail/condition_variable.h
	coro/detail/shared_mutex.h
	coro/detail/semaphore.h
	detail/app_utls.h
	detail/lock_free_queue.h
	detail/string_list.h
	detail/value.h
	detail/ini.h
	detail/execution.h
	detail/library_impl.hii
	detail/library.h
)

set(all_files
	${${target_name}_sources}
	${${target_name}_headers}
	${${target_name}_detail_headers}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_files})

add_library(${target_name} SHARED ${all_files})
target_compile_definitions(${target_name} PRIVATE ${target_name}_EXPORTS)

if (UNIX)
	target_link_libraries(${target_name} PUBLIC pthread dl)
elseif (WIN32)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		target_link_libraries(${target_name} PUBLIC ws2_32 wsock32)
	endif ()
endif()

set_target_properties(${target_name} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${output_dir}/bin
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
install(TARGETS ${target_name} DESTINATION ${install_dir})