set(${PRO_NAME}_3rd_path ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)

include_directories(
	${${PRO_NAME}_3rd_path}/nlohmann.json
	${${PRO_NAME}_3rd_path}/spdlog
	${${PRO_NAME}_3rd_path}/asio
)
