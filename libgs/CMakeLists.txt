include(project)

add_subdirectory(core)
add_subdirectory(coro)
add_subdirectory(http)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
	core.h  io.h  http.h
)