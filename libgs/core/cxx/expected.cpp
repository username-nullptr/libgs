// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "expected.h"
#if !LIBGS_HAS_STD_EXPECTED

namespace libgs
{

const char *bad_expected_access<void>::what() const noexcept
{
	return "bad access to libgs::expected without an expected value";
}

} //namespace libgs

#endif //LIBGS_HAS_STD_EXPECTED
