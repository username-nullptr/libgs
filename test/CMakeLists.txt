set(target_name libgs.test)
add_executable(${target_name} main.cpp)

target_link_libraries(${target_name} PRIVATE gs.http)
target_link_libraries(${target_name} PRIVATE gs.utils)

set_target_properties(${target_name} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
)

function(add_libgs_test name source library)
	set(test_target libgs.test.${name})
	add_executable(${test_target} ${source})

	target_link_libraries(${test_target} PRIVATE ${library})
	target_include_directories(${test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	set_target_properties(${test_target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
	)
	add_test(NAME libgs.${name} COMMAND $<TARGET_FILE:${test_target}>)
endfunction()

add_libgs_test(core core/basic.cpp gs.core)
add_libgs_test(core.facilities core/facilities.cpp gs.core)
add_libgs_test(core.runtime core/runtime.cpp gs.core)
add_libgs_test(coro coro/basic.cpp gs.core)
add_libgs_test(http http/basic.cpp gs.http)
add_libgs_test(http.protocol http/protocol.cpp gs.http)
add_libgs_test(http.loopback http/loopback.cpp gs.http)

set_tests_properties(libgs.http.loopback PROPERTIES TIMEOUT 15)

add_libgs_test(websocket.protocol websocket/protocol.cpp gs.websocket)

add_libgs_test(utils utils/basic.cpp gs.utils)
add_libgs_test(utils.runtime utils/runtime.cpp gs.utils)
add_libgs_test(utils.modules utils/modules.cpp gs.utils)
