set(target_name test)

add_executable(${target_name} main.cpp)
set(OpenSSL_DIR /opt/openssl/1.1.1j/install)

if (CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag("-Wa,-mbig-obj" GNU_BIG_OBJ_FLAG_ENABLE)
	message(STATUS GNU_BIG_OBJ_FLAG_ENABLE=${GNU_BIG_OBJ_FLAG_ENABLE})
endif ()

target_include_directories(${target_name} PRIVATE ${OpenSSL_DIR}/include)
target_link_directories(${target_name} PRIVATE ${OpenSSL_DIR}/lib)
target_link_libraries(${target_name} PRIVATE gs_core)

target_compile_options(${target_name} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
	$<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>
)
set_target_properties(${target_name} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
