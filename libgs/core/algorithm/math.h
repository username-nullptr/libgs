
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_CORE_ALGORITHM_MATH_H
#define LIBGS_CORE_ALGORITHM_MATH_H

#include <libgs/core/global.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_TAPI bool equality (
	concepts::number_type auto a, concepts::number_type auto b
);

[[nodiscard]] LIBGS_CORE_TAPI bool nequality (
	concepts::number_type auto a, concepts::number_type auto b
);

template <typename Iter>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end) requires
	std::is_arithmetic_v<std::remove_reference_t<decltype(*begin)>>;

template <typename Iter>
[[nodiscard]] LIBGS_CORE_TAPI auto mean(Iter begin, Iter end, auto &&func) requires
	std::is_arithmetic_v<decltype(func(*begin))>;

template <typename Iter, typename Data>
[[nodiscard]] LIBGS_CORE_TAPI auto func_inf_pt(Iter begin, Iter end, const Data &threshold, auto &&func) requires
	std::is_same_v<decltype(func(*begin)), Data> and std::is_arithmetic_v<Data>;

template <typename Iter, typename Data>
[[nodiscard]] LIBGS_CORE_TAPI auto func_inf_pt(Iter begin, Iter end, const Data &threshold) requires
	std::is_same_v<decltype(*begin), Data> and std::is_arithmetic_v<Data>;

} //namespace libgs
#include <libgs/core/algorithm/detail/math.h>


#endif //LIBGS_CORE_ALGORITHM_MATH_H