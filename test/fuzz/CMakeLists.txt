# SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

function(add_libgs_websocket_fuzzer name source)
	set(target libgs.fuzz.websocket.${name})
	add_executable(${target} ${source})
	target_link_libraries(${target} PRIVATE gs.websocket)
	target_compile_options(${target} PRIVATE
		-fsanitize=fuzzer,address,undefined
		-fno-omit-frame-pointer
	)
	target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
	set_target_properties(${target} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/fuzz
	)
endfunction()

add_libgs_websocket_fuzzer(frame-parser websocket_frame_parser.cpp)
add_libgs_websocket_fuzzer(handshake-parser websocket_handshake_parser.cpp)
