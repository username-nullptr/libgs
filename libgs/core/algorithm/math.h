// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ALGORITHM_MATH_H
#define LIBGS_CORE_ALGORITHM_MATH_H

#include <libgs/core/global.h>
#include <cmath>

namespace libgs
{

template <typename Iter>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end) requires
	concepts::arithmetic_p<decltype(*begin)>;

template <typename Iter>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end, auto &&func) requires (
	concepts::arithmetic_p<decltype(*func(*begin))> or
	concepts::arithmetic_p<decltype(*func(begin))>
);

} //namespace libgs
#include <libgs/core/algorithm/detail/math.h>


#endif //LIBGS_CORE_ALGORITHM_MATH_H
