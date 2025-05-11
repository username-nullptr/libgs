set(target_name gs_http)
add_project(${target_name} gs_core)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_HTTP_SHARED
)