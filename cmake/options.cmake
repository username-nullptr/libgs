option(LIBGS_BUILD_STATIC
	"-- ${PRO_NAME}: Build static libraries." OFF
)
if (NOT LIBGS_BUILD_STATIC)
	option(LIBGS_ADD_LIBRARY_VERSION
		"-- ${PRO_NAME}: Add version information to library names." ON
	)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	option(LIBGS_USE_LIBCXX
		"-- ${PRO_NAME}: Use clang libcxx." OFF
	)
	if (LIBGS_USE_LIBCXX)
		message(STATUS "${PRO_NAME}: Use clang libcxx.")
		add_compile_options(-stdlib=libc++)
		link_libraries(c++ c++abi)
	endif ()

	option(LIBGS_USE_LLD
		"-- ${PRO_NAME}: Use clang lld." OFF
	)
	if (LIBGS_USE_LLD)
		message(STATUS "${PRO_NAME}: Use clang lld.")
		set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
	endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	option(LIBGS_ENABLE_LTO
		"-- ${PRO_NAME}: Use gnu-lto." OFF
	)
	if (LIBGS_ENABLE_LTO)
		message(STATUS "${PRO_NAME}: Use gnu-lto.")
		add_compile_options(-flto)
	endif ()
endif ()

option(LIBGS_OPENSSL_SUPPORT
	"-- ${PRO_NAME}: OpenSSL support." OFF
)
if (LIBGS_OPENSSL_SUPPORT)
	message(STATUS "${PRO_NAME}: Enable OpenSSL support.")
	add_definitions(-DLIBGS_OPENSSL_SUPPORT=1)
endif ()

set(LIBGS_CONFIG_INCLUDE
	${output_dir}/config_include
)
set(LIBGS_CONFIG_INCLUDE
	${LIBGS_CONFIG_INCLUDE} CACHE PATH
	"Path to libgs config include directory."
)
configure_file (
	${PROJECT_SOURCE_DIR}/libgs/core/cxx/configs.h.in
	${LIBGS_CONFIG_INCLUDE}/libgs/core/cxx/configs.h
	@ONLY
)
include_directories(${LIBGS_CONFIG_INCLUDE})

install(FILES
	${LIBGS_CONFIG_INCLUDE}/libgs/core/cxx/configs.h
	DESTINATION include
)