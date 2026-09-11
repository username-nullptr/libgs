# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(${PRO_NAME}_3rd_path ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)

include_directories(SYSTEM
	${${PRO_NAME}_3rd_path}/nlohmann.json
	${${PRO_NAME}_3rd_path}/spdlog
	${${PRO_NAME}_3rd_path}/asio
)
