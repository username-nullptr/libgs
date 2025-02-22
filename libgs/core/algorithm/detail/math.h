
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_MATH_H
#define LIBGS_CORE_ALGORITHM_DETAIL_MATH_H

namespace libgs
{

template <typename Iter>
auto mean(Iter begin, Iter end) requires
	std::is_arithmetic_v<std::remove_cvref_t<decltype(*begin)>>
{
	return mean(begin, end, [](auto &x){return &x;});
}

template <typename Iter>
auto mean(Iter begin, Iter end, auto &&func) requires (
	std::is_arithmetic_v<std::remove_cvref_t<decltype(*func(*begin))>> or
	std::is_arithmetic_v<std::remove_cvref_t<decltype(*func(begin))>>
){
	using sum_t = decltype(*func(begin));
	auto sum = static_cast<sum_t>(0);
	auto count = static_cast<sum_t>(0);

	for(auto it=begin; it!=end; ++it)
	{
		auto p = func(it);
		if( not p )
			continue;
		sum += *p;
		++count;
	}
	return sum / count;
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MATH_H