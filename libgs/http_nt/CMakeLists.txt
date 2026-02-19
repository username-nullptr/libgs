set(target_name gs.http.nt)
add_project(${target_name} gs.core)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_HTTP_NT_SHARED
)