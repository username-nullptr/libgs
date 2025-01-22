
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

bool equality(concepts::number_type auto a, concepts::number_type auto b)
{
	if constexpr( is_float_v<decltype(a)> or is_float_v<decltype(b)> )
	{
		auto e = std::abs(a - b) ;
		return e < std::numeric_limits<decltype(e)>::epsilon();
	}
	else
		return a == b;
}

bool nequality(concepts::number_type auto a, concepts::number_type auto b)
{
	return not equality(a,b);
}

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
		if( p )
			sum += *p;
		count++;
	}
	return sum / count;
}

namespace detail
{

template <typename Iter, typename C, typename Func>
class LIBGS_CORE_TAPI func_inf_pt
{
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
	explicit func_inf_pt(const C &threshold, const C &precision, Func &func) :
		threshold(threshold), precision(precision), func(func) {}

	[[nodiscard]] auto begin_check(Iter &it, Iter begin, Iter end)
	{
		auto data = func(*begin);
		pre_data = data;

		for(; it!=end; ++it)
		{
			data = func(*it);
			auto check_threshold = [&]
			{
				if( data - pre_data > threshold )
				{
					dtn = direction::rising_edge;
					last_ipt = inf_pt_t::trough;
				}
				else if( data - pre_data < -threshold )
				{
					dtn = direction::falling_edge;
					last_ipt = inf_pt_t::crest;
				}
				else
					return false;

				list.emplace_back(pre_data);
				list.emplace_back(data);
				return true;
			};
			if( check_threshold() )
				break;
			else if( not (std::abs(data - pre_data) > precision) )
			{
				pre_data = data;
				continue;
			}
			for(++it; it!=end; ++it)
			{
				data = func(*it);
				if( not (std::abs(data - pre_data) > precision) )
				{
					pre_data = data;
					break;
				}
				if( check_threshold() )
					break;
			}
			if( not list.empty() )
				break;
		}
		return data;
	}

	void end_check(Iter mit, Iter &it)
	{
		auto data = func(*it);
		pre_data = data;

		for(; it!=mit; --it)
		{
			data = func(*it);
			if( std::abs(data - pre_data) > threshold )
			{
				last = pre_data;
				break;
			}
			else if( not (std::abs(data - pre_data) > precision) )
			{
				pre_data = data;
				continue;
			}
			if( --it == mit )
				break;
			for(; it!=mit; --it)
			{
				data = func(*it);
				if( not (std::abs(data - pre_data) > precision) )
				{
					pre_data = data;
					break;
				}
				else if( std::abs(data - pre_data) > threshold )
				{
					last = pre_data;
					break;
				}
			}
			if( last )
				break;
		}
		// ++it;
	}

	void status_check(direction d, inf_pt_t t, auto &&diff)
	{
		if( last_ipt == t and diff(pre_data, list.back()) > precision )
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
	const C &precision;
	Func &func;

	data_t pre_data;
	std::list<data_t> list;
	std::optional<data_t> last;

	direction dtn = direction::level;
	inf_pt_t last_ipt = {};
};

} //namespace detail

template <typename Iter>
auto func_inf_pt(Iter begin, Iter end, const auto &threshold, double threshold_precision, auto &&func)
	requires concepts::func_inf_pt<Iter, decltype(threshold), decltype(func)>
{
	if( not (threshold > 0) or not (threshold_precision < 1.0) )
	{
		throw std::invalid_argument (
			"libgs::func_inf_pt: threshold and precision must be positive and threshold > precision"
		);
	}
	using data_t = decltype(func(*begin));
	using vector_t = std::vector<data_t>;

	auto it = std::next(begin);
	if( it == end )
		return vector_t{};

	using fip_t = detail::func_inf_pt <
		Iter, decltype(threshold * threshold_precision), decltype(func)
	>;
	using direction = typename fip_t::direction;
	using inf_pt_t = typename fip_t::inf_pt_t;

	auto precision = threshold * threshold_precision;
	fip_t fip(threshold, precision, func);

	auto data = fip.begin_check(it, begin, end);
	if( it == end )
		return vector_t{ fip.list.begin(), fip.list.end() };

	fip.end_check(--it, --end);
	if( fip.list.empty() )
		return vector_t{};

	fip.pre_data = data;
	for(++it; it!=end; ++it)
	{
		data = func(*it);
		if( data - fip.pre_data > precision )
		{
			fip.status_check(direction::rising_edge, inf_pt_t::trough,
				[](auto pre, auto back){ return pre - back; }
			);
		}
		else if( data - fip.pre_data < -precision )
		{
			fip.status_check(direction::falling_edge, inf_pt_t::crest,
				[](auto pre, auto back){ return back - pre; }
			);
		}
		else if( fip.dtn != direction::level )
		{
			auto ipt = fip.dtn == direction::rising_edge ? inf_pt_t::crest : inf_pt_t::trough;
			fip.status_check(direction::level, ipt,
				[](auto pre, auto back){ return std::abs(pre - back); }
			);
		}
		fip.pre_data = data;
	}
	if( fip.last )
	{
		if( fip.list.size() > 1 )
		{
			auto lit = --fip.list.end();
			auto second = *lit;
			auto first = *--lit;

			if( second > first )
			{
				if( *fip.last > second )
					fip.list.pop_back();
			}
			else if( *fip.last < second )
				fip.list.pop_back();
		}
		fip.list.emplace_back(*fip.last);
	}
	return vector_t{ fip.list.begin(), fip.list.end() };
}

template <typename Iter>
auto func_inf_pt(Iter begin, Iter end, const auto &threshold, double threshold_precision)
	requires concepts::func_inf_pt<Iter, decltype(threshold), decltype([](auto x){return x;})>
{
	return func_inf_pt(begin, end, threshold, threshold_precision, [](auto x){return x;});
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MATH_H