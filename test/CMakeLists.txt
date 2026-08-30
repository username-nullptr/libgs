set(target_name libgs.test)

add_executable(${target_name} main.cpp)

if (CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag("-Wa,-mbig-obj" GNU_BIG_OBJ_FLAG_ENABLE)
	message(STATUS GNU_BIG_OBJ_FLAG_ENABLE=${GNU_BIG_OBJ_FLAG_ENABLE})
endif ()

#target_link_libraries(${target_name} PRIVATE gs.core)
target_link_libraries(${target_name} PRIVATE gs.http)
target_link_libraries(${target_name} PRIVATE gs.utils)

target_compile_options(${target_name} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
	$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>
)
set_target_properties(${target_name} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
)

set(http_io_token_test libgs.http_io_token_test)
add_executable(${http_io_token_test} http_io_token_test.cpp)
target_link_libraries(${http_io_token_test} PRIVATE gs.http gs.utils)
set_target_properties(${http_io_token_test} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${LIBGS_OUTPUT_DIR}/bin
)
