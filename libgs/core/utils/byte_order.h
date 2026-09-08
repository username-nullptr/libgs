// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_BYTE_ORDER_H
#define LIBGS_CORE_UTILS_BYTE_ORDER_H

#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/concepts.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_VAPI bool is_little_endian();
[[nodiscard]] LIBGS_CORE_VAPI bool is_big_endian();

[[nodiscard]] LIBGS_CORE_TAPI auto hton(concepts::arithmetic_p auto t);
[[nodiscard]] LIBGS_CORE_TAPI auto hton(concepts::enumerate_p auto e);
LIBGS_CORE_TAPI auto *hton(auto *data, size_t len = 1);

[[nodiscard]] LIBGS_CORE_TAPI auto ntoh(concepts::arithmetic_p auto t);
[[nodiscard]] LIBGS_CORE_TAPI auto ntoh(concepts::enumerate_p auto e);
LIBGS_CORE_TAPI auto *ntoh(auto *data, size_t len = 1);

[[nodiscard]] LIBGS_CORE_TAPI auto reverse(concepts::arithmetic_p auto t);
[[nodiscard]] LIBGS_CORE_TAPI auto reverse(concepts::enumerate_p auto e);
LIBGS_CORE_TAPI auto *reverse(auto *data, size_t len = 1);

[[nodiscard]] LIBGS_CORE_TAPI auto to_big_endian(concepts::arithmetic_p auto t);
[[nodiscard]] LIBGS_CORE_TAPI auto to_big_endian(concepts::enumerate_p auto e);
LIBGS_CORE_TAPI auto *to_big_endian(auto *data, size_t len = 1);

[[nodiscard]] LIBGS_CORE_TAPI auto to_little_endian(concepts::arithmetic_p auto t);
[[nodiscard]] LIBGS_CORE_TAPI auto to_little_endian(concepts::enumerate_p auto e);
LIBGS_CORE_TAPI auto *to_little_endian(auto *data, size_t len = 1);

} //namespace libgs
#include <libgs/core/utils/detail/byte_order.h>


#endif //LIBGS_CORE_UTILS_BYTE_ORDER_H
