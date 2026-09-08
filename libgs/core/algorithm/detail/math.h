// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_MATH_H
#define LIBGS_CORE_ALGORITHM_DETAIL_MATH_H

namespace libgs
{

template <typename Iter>
auto mean(Iter begin, Iter end) requires
	concepts::arithmetic_p<decltype(*begin)>
{
	return mean(begin, end, [](auto &x){return x;});
}

template <typename Iter>
auto mean(Iter begin, Iter end, auto &&func) requires (
	concepts::arithmetic_p<decltype(*func(*begin))> or
	concepts::arithmetic_p<decltype(*func(begin))>
){
	using sum_t = std::remove_cvref_t<decltype(*func(begin))>;
	auto sum = static_cast<sum_t>(0);
	auto count = static_cast<sum_t>(0);

	for(auto it=begin; it!=end; ++it)
	{
		auto p = func(it);
		using p_t = std::remove_cvref_t<decltype(p)>;

		if constexpr( std::is_pointer_v<p_t> )
		{
			if( not p )
				continue;
		}
		sum += *p;
		++count;
	}
	return sum / count;
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_MATH_H
