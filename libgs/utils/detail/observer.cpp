// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "libgs/utils/observer.h"

namespace libgs::utils::detail
{

observer::map_t &observer::map() noexcept
{
	static map_t map;
	return map;
}

spin_mutex &observer::mutex() noexcept
{
	static spin_mutex mutex;
	return mutex;
}

} //nnamespace libgs::utils::detail
