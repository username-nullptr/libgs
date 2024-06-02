add_executable(test main.cpp)
set(OpenSSL_DIR /opt/openssl/install)

target_include_directories(test PRIVATE ${OpenSSL_DIR}/include)
target_link_directories(test PRIVATE ${OpenSSL_DIR}/lib)
target_link_libraries(test PRIVATE gs_io)

set_target_properties(test PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${output_dir}/bin
)
