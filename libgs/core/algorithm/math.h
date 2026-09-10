// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ALGORITHM_MATH_H
#define LIBGS_CORE_ALGORITHM_MATH_H

#include <libgs/core/global.h>
#include <cmath>

namespace libgs { namespace concepts
{

template <typename Iter, typename Func>
concept mean_value_projection = requires(Iter it, Func &func) {
	{ *func(*it) } -> arithmetic_p;
};

template <typename Iter, typename Func>
concept mean_iterator_projection = requires(Iter it, Func &func) {
	{ *func(it) } -> arithmetic_p;
};

} //namespace concepts

template <typename Iter>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end) requires
	concepts::arithmetic_p<decltype(*begin)>;

template <typename Iter, typename Func>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end, Func &&func) requires (
	concepts::mean_value_projection<Iter,Func> or concepts::mean_iterator_projection<Iter,Func>
);

} //namespace libgs
#include <libgs/core/algorithm/detail/math.h>


#endif //LIBGS_CORE_ALGORITHM_MATH_H
