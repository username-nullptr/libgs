# SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
# SPDX-License-Identifier: MIT

set(target_name gs.coro)
add_project(${target_name} gs.core)

target_compile_definitions (
	${target_name} PRIVATE LIBGS_CORO_SHARED
)
