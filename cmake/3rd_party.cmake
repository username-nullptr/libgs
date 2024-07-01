set(libgs_3rd_path ${CMAKE_SOURCE_DIR}/3rd_party)

include_directories(
	${libgs_3rd_path}/nlohmann.json
	${libgs_3rd_path}/spdlog
	${libgs_3rd_path}/asio
)
