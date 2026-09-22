# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

include(project)


function(libgs_install_module_headers module)
	install(FILES ${module}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libgs)
	install(DIRECTORY ${module}/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libgs/${module}
		FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.ipp"
		${ARGN}
	)
endfunction()


source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES core.h)
add_subdirectory(core)
# This PIMPL definition is compiled into gs.core and is not included by any
# public header.
libgs_install_module_headers(core
	PATTERN "library_impl.ipp" EXCLUDE
)

if (LIBGS_BUILD_CORO)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES coro.h)
	add_subdirectory(coro)
	libgs_install_module_headers(coro)
endif ()

if (LIBGS_BUILD_HTTP)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES http.h)
	add_subdirectory(http)
	# The base parser is an implementation detail used only by the client and
	# server parser translation units. Other detail headers contain public
	# template definitions and must remain in the install tree.
	libgs_install_module_headers(http
		REGEX "/protocol/utils/core/parser[.]h$" EXCLUDE
		REGEX "/protocol/utils/core/detail/parser[.]h$" EXCLUDE
	)
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
