
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

#include <cmath>

namespace libgs
{

template <typename Iter>
auto mean(Iter begin, Iter end) requires
	std::is_arithmetic_v<std::remove_reference_t<decltype(*begin)>>
{
	return mean(begin, end, [](auto x){return x;});
}

template <typename Iter>
auto mean(Iter begin, Iter end, auto &&func) requires
	std::is_arithmetic_v<decltype(func(*begin))>
{
	using sum_t = decltype(func(*begin));
	auto sum = static_cast<sum_t>(0);
	auto count = static_cast<sum_t>(0);

	for(auto it=begin; it!=end; ++it)
	{
		sum += func(*it);
		count++;
	}
	return sum / count;
}

template <typename Iter, typename Data>
auto func_inf_pt(Iter begin, Iter end, const Data &threshold, auto &&func) requires
	std::is_same_v<decltype(func(*begin)), Data> and std::is_arithmetic_v<Data>
{
	using vector_t = std::vector<Data>;
	auto it = std::next(begin);

	if( it == end )
		return vector_t{};

	enum class direction {
		level, rising_edge, falling_edge
	}
	dtn = direction::level;

	enum class inf_pt_t {
		crest, trough
	}
	last_ipt = inf_pt_t::crest;

	auto pre_data = func(*begin);
	std::list<Data> list;

	auto status_check = [&](direction d, inf_pt_t t, auto &&comp, auto &&diff) mutable
	{
		if( dtn == d )
			return ;
		else if( list.empty() )
			list.emplace_back(pre_data);
		else if( last_ipt == t and comp(pre_data, list.back()) )
		{
			list.pop_back();
			list.emplace_back(pre_data);
		}
		else if( diff(pre_data, list.back()) > threshold )
		{
			list.emplace_back(pre_data);
			last_ipt = t;
		}
		dtn = d;
	};
	for(; it!=end; ++it)
	{
		auto data = func(*it);
		if( data > pre_data )
		{
			status_check(direction::rising_edge, inf_pt_t::trough,
				[](double pre, double back){ return pre < back; },
				[](double pre, double back){ return back - pre; }
			);
		}
		else if( data < pre_data )
		{
			status_check(direction::falling_edge, inf_pt_t::crest,
				[](double pre, double back){ return pre > back; },
				[](double pre, double back){ return pre - back; }
			);
		}
		else
			dtn = direction::level;
		pre_data = data;
	}
	if( dtn == direction::level )
	{
		if( not list.empty() and std::abs(list.back() - pre_data) < threshold )
			list.pop_back();
		list.emplace_back(pre_data);
	}
	return vector_t{ list.begin(), list.end() };
}

template <typename Iter, typename Data>
auto func_inf_pt(Iter begin, Iter end, const Data &threshold) requires
	std::is_same_v<decltype(*begin), Data> and std::is_arithmetic_v<Data>
{
	return func_inf_pt(begin, end, threshold, [](auto x){return x;});
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MATH_H