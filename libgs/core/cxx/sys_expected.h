// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_SYS_EXPECTED_H
#define LIBGS_CORE_CXX_SYS_EXPECTED_H

#include <libgs/core/cxx/expected.h>

namespace libgs
{

template <concepts::expected_value Value = void>
using sys_expected = expected<Value,error_code>;

using sys_unexpected = unexpected<error_code>;

template <concepts::optional_value_p Value>
[[nodiscard]] LIBGS_CORE_TAPI auto make_sys_expected(Value &&value);
[[nodiscard]] LIBGS_CORE_VAPI sys_expected<> make_sys_expected();

template <concepts::expected_value Value = void>
LIBGS_CORE_TAPI void sys_expected_loc_throw(const sys_expected<Value> &expected,
	std::source_location loc = std::source_location::current()
);

template <concepts::expected_value Value = void>
LIBGS_CORE_TAPI void sys_expected_loc_throw(const sys_expected<Value> &expected,
	concepts::text_p<char> auto &&msg, std::source_location loc = std::source_location::current()
);

using io_expected = sys_expected<size_t>;
using io_unexpected = sys_unexpected;

[[nodiscard]] LIBGS_CORE_TAPI io_expected make_io_expected(size_t sum);

} //namespace libgs
#include <libgs/core/cxx/detail/sys_expected.h>


#endif //LIBGS_CORE_CXX_SYS_EXPECTED_H
