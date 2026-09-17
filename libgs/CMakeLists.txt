# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

include(project)


function(libgs_install_module_headers module)
	install(FILES ${module}.h DESTINATION include/libgs/)
	install(DIRECTORY ${module}/ DESTINATION include/libgs/${module}/
		FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.ipp"
	)
endfunction()


source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES core.h)
add_subdirectory(core)
libgs_install_module_headers(core)

if (LIBGS_BUILD_CORO)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES coro.h)
	add_subdirectory(coro)
	libgs_install_module_headers(coro)
endif ()

if (LIBGS_BUILD_HTTP)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES http.h)
	add_subdirectory(http)
	libgs_install_module_headers(http)
endif ()

if (LIBGS_BUILD_WEBSOCKET)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES websocket.h)
	add_subdirectory(websocket)
	libgs_install_module_headers(websocket)
endif ()

if (LIBGS_BUILD_UTILITIES)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES utils.h)
	add_subdirectory(utils)
	libgs_install_module_headers(utils)
endif ()

if (LIBGS_ENABLE_TEST_SANITIZERS OR LIBGS_ENABLE_TEST_TSAN)
	foreach(target gs.core gs.coro gs.http gs.websocket gs.utils)
		if (TARGET ${target})
			target_link_libraries(${target} PUBLIC libgs.test.sanitizer)
		endif ()
	endforeach()
endif ()
