# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

include(CMakeParseArguments)

if(CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag("-Wa,-mbig-obj" LIBGS_EXAMPLE_GNU_BIG_OBJ)
endif()


function(libgs_add_example category name)
	set(options)
	set(one_value_args)
	set(multi_value_args SOURCES LIBRARIES)

	cmake_parse_arguments(EXAMPLE
		"${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
	)
	if(NOT EXAMPLE_SOURCES)
		set(EXAMPLE_SOURCES "${category}/${name}.cpp")
	endif()

	set(target "libgs.example.${category}.${name}")
	add_executable(${target} ${EXAMPLE_SOURCES})

	if (category MATCHES "^(http|websocket)$")
		libgs_limit_heavy_compile(${target})
	endif ()

	target_link_libraries(${target} PRIVATE ${EXAMPLE_LIBRARIES})
	target_compile_options(${target} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
		$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${LIBGS_EXAMPLE_GNU_BIG_OBJ}>>:-Wa,-mbig-obj>
	)
	set_target_properties(${target} PROPERTIES
		OUTPUT_NAME ${name}
		RUNTIME_OUTPUT_DIRECTORY "${LIBGS_OUTPUT_DIR}/examples/${category}"
	)
	install(TARGETS ${target}
		RUNTIME DESTINATION "examples/${category}"
	)
endfunction()


# Core
libgs_add_example(core algorithms      LIBRARIES gs.core)
libgs_add_example(core app_paths       LIBRARIES gs.core)
libgs_add_example(core args_parser     LIBRARIES gs.core)
libgs_add_example(core execution       LIBRARIES gs.core)
libgs_add_example(core ini             LIBRARIES gs.core)
libgs_add_example(core lock_free_queue LIBRARIES gs.core)
libgs_add_example(core value           LIBRARIES gs.core)

add_library(libgs.example.core.dynamic_library_plugin SHARED
	core/dynamic_library_plugin.cpp
)
set_target_properties(libgs.example.core.dynamic_library_plugin PROPERTIES
	OUTPUT_NAME dynamic_library_plugin
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${LIBGS_OUTPUT_DIR}/examples/core"
	RUNTIME_OUTPUT_DIRECTORY "${LIBGS_OUTPUT_DIR}/examples/core"
)
install(TARGETS libgs.example.core.dynamic_library_plugin
	LIBRARY DESTINATION examples/core
	RUNTIME DESTINATION examples/core
)
libgs_add_example(core dynamic_library LIBRARIES gs.core)
add_dependencies(libgs.example.core.dynamic_library
	libgs.example.core.dynamic_library_plugin
)

# Coroutines
if (LIBGS_BUILD_CORO)
	libgs_add_example(coro basics             LIBRARIES gs.coro)
	libgs_add_example(coro mutex              LIBRARIES gs.coro)
	libgs_add_example(coro shared_mutex       LIBRARIES gs.coro)
	libgs_add_example(coro semaphore          LIBRARIES gs.coro)
	libgs_add_example(coro condition_variable LIBRARIES gs.coro)
endif ()

# HTTP/1.0 and HTTP/1.1
if (LIBGS_BUILD_HTTP)
	libgs_add_example(http client_sync      LIBRARIES gs.http)
	libgs_add_example(http client_awaitable LIBRARIES gs.http)
	libgs_add_example(http client_cookies   LIBRARIES gs.http)
	libgs_add_example(http client_file      LIBRARIES gs.http)
	libgs_add_example(http proxy_client     LIBRARIES gs.http)
	libgs_add_example(http protocol         LIBRARIES gs.http)
	libgs_add_example(http server           LIBRARIES gs.http)
	libgs_add_example(http server_aop       LIBRARIES gs.http)
	libgs_add_example(http server_file      LIBRARIES gs.http)
	libgs_add_example(http server_session   LIBRARIES gs.http)

	if (LIBGS_OPENSSL_SUPPORT)
		libgs_add_example(http https_server LIBRARIES gs.http)
	endif ()
endif ()

# WebSocket protocol
if (LIBGS_BUILD_WEBSOCKET)
	libgs_add_example(websocket client            LIBRARIES gs.websocket)
	libgs_add_example(websocket proxy_client      LIBRARIES gs.websocket)
	libgs_add_example(websocket retry_open        LIBRARIES gs.websocket)
	libgs_add_example(websocket server            LIBRARIES gs.websocket)
	libgs_add_example(websocket protocol          LIBRARIES gs.websocket)
	libgs_add_example(websocket mixed_http_client LIBRARIES gs.websocket)
	libgs_add_example(websocket mixed_http_server LIBRARIES gs.websocket)

	if (LIBGS_OPENSSL_SUPPORT)
		libgs_add_example(websocket wss_client LIBRARIES gs.websocket)
		libgs_add_example(websocket wss_server LIBRARIES gs.websocket)
	endif ()
endif ()

# Application utilities
if (LIBGS_BUILD_UTILITIES)
	libgs_add_example(utils logger             LIBRARIES gs.utils)
	libgs_add_example(utils observer           LIBRARIES gs.utils)
	libgs_add_example(utils process            LIBRARIES gs.utils)
	libgs_add_example(utils soft_bus_local     LIBRARIES gs.utils)
	libgs_add_example(utils soft_bus_transport LIBRARIES gs.utils)
	libgs_add_example(utils settings           LIBRARIES gs.utils)
	libgs_add_example(utils signal_slot        LIBRARIES gs.utils)
	libgs_add_example(utils modules
		SOURCES
			utils/modules/main.cpp
			utils/modules/foundation.cpp
			utils/modules/cache.cpp
			utils/modules/api.cpp
		LIBRARIES gs.utils
	)
endif ()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
	core/algorithms.cpp
	core/app_paths.cpp
	core/args_parser.cpp
	core/dynamic_library.cpp
	core/dynamic_library_plugin.cpp
	core/execution.cpp
	core/ini.cpp
	core/lock_free_queue.cpp
	core/value.cpp
	coro/basics.cpp
	coro/condition_variable.cpp
	coro/mutex.cpp
	coro/semaphore.cpp
	coro/shared_mutex.cpp
	http/client_awaitable.cpp
	http/client_cookies.cpp
	http/client_file.cpp
	http/client_sync.cpp
	http/https_server.cpp
	http/protocol.cpp
	http/proxy_client.cpp
	http/server.cpp
	http/server_aop.cpp
	http/server_file.cpp
	http/server_session.cpp
	websocket/mixed_http_client.cpp
	websocket/mixed_http_server.cpp
	websocket/protocol.cpp
	websocket/client.cpp
	websocket/proxy_client.cpp
	websocket/retry_open.cpp
	websocket/server.cpp
	websocket/wss_client.cpp
	websocket/wss_server.cpp
	utils/logger.cpp
	utils/modules/api.cpp
	utils/modules/cache.cpp
	utils/modules/foundation.cpp
	utils/modules/main.cpp
	utils/observer.cpp
	utils/process.cpp
	utils/soft_bus_local.cpp
	utils/soft_bus_transport.cpp
	utils/settings.cpp
	utils/signal_slot.cpp
)
