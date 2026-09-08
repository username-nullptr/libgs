// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "libgs/utils/process.h"
#include <libgs/core/system/app_utls.h>

namespace libgs::utils::detail
{

sys_expected<uint64_t> process::set_single(std::string_view key)
{
	return app::home_directory().and_then([&](const auto &path) mutable {
		return set_single(path, key);
	});
}

} //namespace libgs::utils::detail
