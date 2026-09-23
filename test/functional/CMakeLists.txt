# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

add_libgs_test(core core/basic.cpp gs.core)
add_libgs_test(core.facilities core/facilities.cpp gs.core)
add_libgs_test(core.runtime core/runtime.cpp gs.core)
add_libgs_test(core.jthread core/jthread.cpp gs.core)
# The native standard-library implementation is trusted. Force the fallback so
# this target always exercises LibGS's compatibility implementation.
target_compile_definitions(libgs.test.core.jthread PRIVATE
	LIBGS_HAS_STD_JTHREAD=0
)

add_library(libgs.test.core.fixture SHARED core/library_fixture.cpp)
set_target_properties(libgs.test.core.fixture PROPERTIES
	PREFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	LIBRARY_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
)
add_libgs_test(core.public-api core/public_api.cpp gs.core
	$<TARGET_FILE:libgs.test.core.fixture>
)
add_dependencies(libgs.test.core.public-api libgs.test.core.fixture)

if (LIBGS_BUILD_CORO)
	add_libgs_test(coro coro/basic.cpp gs.coro)
endif ()

if (LIBGS_BUILD_HTTP)
	add_libgs_test(http http/basic.cpp gs.http)
	add_libgs_test(http.protocol http/protocol.cpp gs.http)

	add_libgs_test(http.public-api http/public_api.cpp gs.http)
	add_libgs_test(http.loopback http/loopback.cpp gs.http)

	if (NOT LIBGS_ENABLE_TEST_SANITIZERS AND NOT LIBGS_ENABLE_TEST_TSAN)
		set_tests_properties(libgs.http.loopback PROPERTIES TIMEOUT 15)
	endif ()
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	add_libgs_test(websocket.protocol websocket/protocol.cpp gs.websocket)
	add_libgs_test(websocket.public-api websocket/public_api.cpp gs.websocket)

	add_libgs_test(websocket.handshake websocket/handshake.cpp gs.websocket)
	add_libgs_test(websocket.client-server websocket/client_server.cpp gs.websocket)

	if (LIBGS_OPENSSL_SUPPORT)
		add_libgs_test(websocket.wss websocket/wss.cpp gs.websocket)
	endif ()

	if (NOT LIBGS_ENABLE_TEST_SANITIZERS AND NOT LIBGS_ENABLE_TEST_TSAN)
		set_tests_properties(libgs.websocket.handshake PROPERTIES TIMEOUT 15)
		set_tests_properties(libgs.websocket.client-server PROPERTIES TIMEOUT 15)
		if (LIBGS_OPENSSL_SUPPORT)
			set_tests_properties(libgs.websocket.wss PROPERTIES TIMEOUT 15)
		endif ()
	endif ()

	add_libgs_test(websocket.stream websocket/stream.cpp gs.websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	add_libgs_test(utils utils/basic.cpp gs.utils)
	add_libgs_test(utils.runtime utils/runtime.cpp gs.utils)
	add_libgs_test(utils.sbus utils/sbus.cpp gs.utils)
	add_libgs_test(utils.modules utils/modules.cpp gs.utils)
endif ()


# Network loopback tests remain the dependency-free baseline. When an
# independent implementation is already available in the build environment,
# add interoperability checks without downloading anything at configure time.
find_package(Python3 3.8 QUIET COMPONENTS Interpreter)

if (Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
	if (LIBGS_BUILD_HTTP)
		find_program(LIBGS_TEST_CURL_EXECUTABLE NAMES curl)

		if (LIBGS_TEST_CURL_EXECUTABLE)
			set(libgs_http_interop_backend curl)
			set(libgs_http_interop_arguments
				--curl ${LIBGS_TEST_CURL_EXECUTABLE}
			)
		else ()
			set(libgs_http_interop_backend python-stdlib)
			set(libgs_http_interop_arguments)
		endif ()

		add_libgs_test_program(libgs.test.http.interop-peer
			http/interop_peer.cpp gs.http
		)
		add_libgs_ctest(libgs.http.interop
			${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/interop/http_interop.py
			--peer $<TARGET_FILE:libgs.test.http.interop-peer>
			--backend ${libgs_http_interop_backend}
			${libgs_http_interop_arguments}
		)
		configure_libgs_functional_test(libgs.http.interop)

		set_property(TEST libgs.http.interop APPEND PROPERTY LABELS interop external)
		set_property(TEST libgs.http.interop PROPERTY TIMEOUT 30)

		message(STATUS
			"${PRO_NAME}: HTTP interoperability backend: ${libgs_http_interop_backend}"
		)
	endif ()

	if (LIBGS_BUILD_WEBSOCKET)
		find_program(LIBGS_TEST_NODE_EXECUTABLE NAMES node nodejs)

		set(libgs_websocket_interop_backend "")
		set(libgs_websocket_interop_arguments)

		if (LIBGS_TEST_NODE_EXECUTABLE)
			execute_process (
				COMMAND ${LIBGS_TEST_NODE_EXECUTABLE} -p "require.resolve('ws')"
				RESULT_VARIABLE libgs_node_ws_result
				OUTPUT_VARIABLE libgs_node_ws_module
				OUTPUT_STRIP_TRAILING_WHITESPACE
				ERROR_QUIET
			)
			if (libgs_node_ws_result EQUAL 0)
				set(libgs_websocket_interop_backend node-ws)
				set(libgs_websocket_interop_arguments
					--node ${LIBGS_TEST_NODE_EXECUTABLE}
					--node-ws-module ${libgs_node_ws_module}
					--node-helper
					${CMAKE_CURRENT_SOURCE_DIR}/interop/websocket_node_ws.js
				)
			endif ()
		endif ()

		if (NOT libgs_websocket_interop_backend)
			execute_process (
				COMMAND ${Python3_EXECUTABLE} -c
					"import websockets; assert hasattr(websockets, 'connect')"
				RESULT_VARIABLE libgs_python_websockets_result
				OUTPUT_QUIET ERROR_QUIET
			)
			if (libgs_python_websockets_result EQUAL 0)
				set(libgs_websocket_interop_backend python-websockets)
			endif ()
		endif ()

		if (NOT libgs_websocket_interop_backend)
			execute_process (
				COMMAND ${Python3_EXECUTABLE} -c
					"import websocket; assert hasattr(websocket, 'create_connection')"
				RESULT_VARIABLE libgs_python_websocket_client_result
				OUTPUT_QUIET ERROR_QUIET
			)
			if (libgs_python_websocket_client_result EQUAL 0)
				set(libgs_websocket_interop_backend python-websocket-client)
			endif ()
		endif ()

		if (NOT libgs_websocket_interop_backend)
			find_program(LIBGS_TEST_WSCAT_EXECUTABLE NAMES wscat)

			if (LIBGS_TEST_WSCAT_EXECUTABLE)
				set(libgs_websocket_interop_backend wscat)
				set(libgs_websocket_interop_arguments
					--wscat ${LIBGS_TEST_WSCAT_EXECUTABLE}
				)
			endif ()
		endif ()

		if (libgs_websocket_interop_backend)
			add_libgs_test_program(libgs.test.websocket.interop-peer
				websocket/interop_peer.cpp gs.websocket
			)
			add_libgs_ctest(libgs.websocket.interop
				${Python3_EXECUTABLE}
				${CMAKE_CURRENT_SOURCE_DIR}/interop/websocket_interop.py
				--peer $<TARGET_FILE:libgs.test.websocket.interop-peer>
				--backend ${libgs_websocket_interop_backend}
				${libgs_websocket_interop_arguments}
			)
			configure_libgs_functional_test(libgs.websocket.interop)

			set_property(TEST libgs.websocket.interop APPEND PROPERTY
				LABELS interop external
			)
			set_property(TEST libgs.websocket.interop PROPERTY TIMEOUT 30)
			message(STATUS
				"${PRO_NAME}: WebSocket interoperability backend: ${libgs_websocket_interop_backend}"
			)
		else ()
			message(STATUS
				"${PRO_NAME}: No external WebSocket client found; using LibGS loopback tests."
			)
		endif ()
	endif ()
else ()
	if (CMAKE_CROSSCOMPILING)
		message(STATUS
			"${PRO_NAME}: External interoperability tests disabled while cross-compiling; using LibGS loopback tests."
		)
	else ()
		message(STATUS
			"${PRO_NAME}: Python 3.8+ not found; using LibGS HTTP/WebSocket loopback tests."
		)
	endif ()
endif ()
