# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

function(add_libgs_fuzzer module name source library)
	set(target libgs.fuzz.${module}.${name})
	add_executable(${target} ${source})
	target_link_libraries(${target} PRIVATE ${library})
	target_compile_options(${target} PRIVATE
		-fsanitize=fuzzer,address,undefined
		-fno-omit-frame-pointer
	)
	target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
	set_target_properties(${target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/fuzz
	)
endfunction()

foreach(target gs.core gs.http gs.websocket)
	if (TARGET ${target})
		target_compile_options(${target} PRIVATE
			-fsanitize=fuzzer-no-link,address,undefined
			-fno-omit-frame-pointer
		)
		target_link_options(${target} PRIVATE -fsanitize=address,undefined)
	endif ()
endforeach()

add_libgs_fuzzer(core url core_url.cpp gs.core)

if (LIBGS_BUILD_HTTP)
	add_libgs_fuzzer(http request-parser http_request_parser.cpp gs.http)
	add_libgs_fuzzer(http response-parser http_response_parser.cpp gs.http)
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	add_libgs_fuzzer(websocket frame-parser websocket_frame_parser.cpp gs.websocket)
	add_libgs_fuzzer(websocket handshake-parser websocket_handshake_parser.cpp gs.websocket)
endif ()
