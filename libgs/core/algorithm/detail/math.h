
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

namespace detail
{

template <typename Iter, typename C, typename Func>
class LIBGS_CORE_TAPI func_inf_pt
{
	LIBGS_DISABLE_COPY_MOVE(func_inf_pt)

public:
	using data_t = decltype (
		std::declval<Func>()(*std::declval<Iter>())
	);
	enum class direction {
		level, rising_edge, falling_edge
	};
	enum class inf_pt_t {
		crest, trough
	};

public:
	explicit func_inf_pt(const C &threshold, Func &func) :
		threshold(threshold), func(func) {}

	[[nodiscard]] Iter find_first(Iter begin, Iter end)
	{
		auto it = begin;
		auto max_it = it;
		auto min_it = it;

		for(; it!=end; ++it)
		{
			auto data = func(*it);
			auto max = *max_it;
			auto min = *min_it;

			if( data > max )
			{
				max_it = it;
				if( equal_less(max - min, threshold) )
					continue;

				pre_data = max;
				list.emplace_back(min);
				list.emplace_back(max);
				dtn = direction::rising_edge;
				last_ipt = inf_pt_t::trough;
				return max_it;
			}
			else if( data < min )
			{
				min_it = it;
				if( equal_less(max - min, threshold) )
					continue;

				pre_data = min;
				list.emplace_back(max);
				list.emplace_back(min);
				dtn = direction::falling_edge;
				last_ipt = inf_pt_t::crest;
				return min_it;
			}
		}
		return end;
	}

public:
	void rising_check()
	{
		status_check(direction::rising_edge, inf_pt_t::trough,
			[](auto pre, auto back){ return pre - back; }
		);
	}

	void falling_check()
	{
		status_check(direction::falling_edge, inf_pt_t::crest,
			[](auto pre, auto back){ return back - pre; }
		);
	}

	void level_check()
	{
		auto ipt = dtn == direction::rising_edge ? inf_pt_t::crest : inf_pt_t::trough;
		status_check(direction::level, ipt,
			[](auto pre, auto back){ return std::abs(pre - back); }
		);
	}

private:
	void status_check(direction d, inf_pt_t t, auto &&diff)
	{
		if( last_ipt == t and diff(pre_data, list.back()) > 0.0 )
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

public:
	const C &threshold;
	Func &func;

	data_t pre_data;
	std::list<data_t> list;

	direction dtn = direction::level;
	inf_pt_t last_ipt = {};
};

} //namespace detail

template <typename Iter>
auto func_inf_pt(Iter begin, Iter end, const auto &threshold, auto &&func)
	requires concepts::func_inf_pt<Iter, decltype(threshold), decltype(func)>
{
	if( equal_less(threshold,0.0) )
	{
		throw std::invalid_argument (
			"libgs::func_inf_pt: Argument 'threshold' must be positive"
		);
	}
	using data_t = decltype(func(*begin));
	using vector_t = std::vector<data_t>;

	using fip_t = detail::func_inf_pt <
		Iter, decltype(threshold), decltype(func)
	>;
	using direction = typename fip_t::direction;

	fip_t fip(threshold, func);
	auto it = fip.find_first(begin, end);

	if( it == end )
	{
		return vector_t {
			fip.list.begin(), fip.list.end()
		};
	}
	for(++it; it!=end; ++it)
	{
		auto data = func(*it);
		if( data > fip.pre_data )
			fip.rising_check();
		else if( data < fip.pre_data )
			fip.falling_check();
		else if( fip.dtn != direction::level )
			fip.level_check();
		fip.pre_data = data;
	}
	if( fip.dtn != direction::level )
		fip.level_check();

	return vector_t {
		fip.list.begin(), fip.list.end()
	};
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MATH_H