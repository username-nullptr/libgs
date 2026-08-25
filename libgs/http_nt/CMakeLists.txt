set(target_name gs.http.nt)
add_project(${target_name} gs.core)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_HTTP_NT_SHARED
)
option(LIBGS_HTTP_NT_ZLIB_SUPPORT
	"-- ${PRO_NAME}: HTTP z-lib support." OFF
)
if (LIBGS_HTTP_NT_ZLIB_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable z-lib support.")
	add_definitions(-DLIBGS_HTTP_NT_ZLIB_SUPPORT=1)

	find_package(ZLIB REQUIRED)
	target_link_libraries(${target_name} PRIVATE ZLIB::ZLIB)
endif ()

set(LIBGS_HTTP_NT_CONFIG_INCLUDE
	${LIBGS_OUTPUT_DIR}/config_include
)
set(LIBGS_HTTP_NT_CONFIG_INCLUDE
	${LIBGS_HTTP_NT_CONFIG_INCLUDE} CACHE PATH
	"Path to libgs config include directory."
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/http_nt/cxx/configs.h.in
	${LIBGS_HTTP_NT_CONFIG_INCLUDE}/libgs/http_nt/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_HTTP_NT_CONFIG_INCLUDE})

install(FILES
	${LIBGS_HTTP_NT_CONFIG_INCLUDE}/libgs/http_nt/cxx/configs.h
	DESTINATION include/libgs/http_nt/cxx
)
