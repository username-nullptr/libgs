add_libgs_test(core core/basic.cpp gs.core)
add_libgs_test(core.facilities core/facilities.cpp gs.core)
add_libgs_test(core.runtime core/runtime.cpp gs.core)

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
	add_libgs_test(coro coro/basic.cpp gs.core)
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
	add_libgs_test(websocket.handshake websocket/handshake.cpp gs.websocket)

	if (NOT LIBGS_ENABLE_TEST_SANITIZERS AND NOT LIBGS_ENABLE_TEST_TSAN)
		set_tests_properties(libgs.websocket.handshake PROPERTIES TIMEOUT 15)
	endif ()

	add_libgs_test(websocket.stream websocket/stream.cpp gs.websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	add_libgs_test(utils utils/basic.cpp gs.utils)
	add_libgs_test(utils.runtime utils/runtime.cpp gs.utils)
	add_libgs_test(utils.sbus utils/sbus.cpp gs.utils)
	add_libgs_test(utils.modules utils/modules.cpp gs.utils)
endif ()
