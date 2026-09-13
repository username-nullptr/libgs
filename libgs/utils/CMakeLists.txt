# SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.utils)
add_project(${target_name} gs.coro)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_UTILS_SHARED
)
