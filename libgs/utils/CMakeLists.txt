set(target_name gs.utils)
add_project(${target_name} gs.core)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_UTILS_SHARED
)
