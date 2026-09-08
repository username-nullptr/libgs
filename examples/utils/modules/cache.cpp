// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/utils/modules.h>
#include <iostream>

LIBGS_UTILS_MODULE_INIT (
	"cache",
	{.parents = {"foundation"}}, []{
		std::cout << "cache initialized after foundation\n";
	}
);
