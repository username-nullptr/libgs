set(target_name gs.websocket)
add_project(${target_name} gs.http)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_WEBSOCKET_SHARED
)
if (WIN32)
	target_link_libraries(${target_name} PRIVATE bcrypt)
endif ()
