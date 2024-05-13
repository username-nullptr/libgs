if (WIN32)
	set(OS_CPP win)
	set(IS_CPP winnt)
elseif (UNIX)
	if (APPLE)
		set(OS_CPP apple)
	else ()
		set(OS_CPP unix)
	endif ()
	set(IS_CPP posix)
endif()

add_subdirectory(core)
add_subdirectory(io)
add_subdirectory(http)
add_subdirectory(web)
add_subdirectory(dbi)
add_subdirectory(utils)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
	core.h  io.h  http.h
)