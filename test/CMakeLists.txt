set(target_name test)

add_executable(${target_name} main.cpp)
set(OpenSSL_DIR /opt/openssl/install)

target_include_directories(${target_name} PRIVATE ${OpenSSL_DIR}/include)
target_link_directories(${target_name} PRIVATE ${OpenSSL_DIR}/lib)
target_link_libraries(${target_name} PRIVATE gs_io)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	target_compile_options(${target_name} PRIVATE -Wl,--default-image-base-low)
	target_link_options(${target_name} PRIVATE -Wl,--default-image-base-low)
endif ()
set_target_properties(${target_name} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
