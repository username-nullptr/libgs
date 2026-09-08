// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_DETAIL_OPERATOR_H
#define LIBGS_CORE_CXX_DETAIL_OPERATOR_H

namespace libgs
{

constexpr bool equality(concepts::arithmetic_p auto a, concepts::arithmetic_p auto b)
{
	if constexpr( is_float_v<decltype(a)> or is_float_v<decltype(b)> )
	{
		auto e = std::abs(a - b) ;
		return e < std::numeric_limits<decltype(e)>::epsilon();
	}
	else
		return a == b;
}

constexpr bool nequality(concepts::arithmetic_p auto a, concepts::arithmetic_p auto b)
{
	return not equality(a,b);
}

constexpr bool equal_greater(concepts::arithmetic_p auto a, concepts::arithmetic_p auto b)
{
	return a > b or equality(a,b);
}

constexpr bool equal_less(concepts::arithmetic_p auto a, concepts::arithmetic_p auto b)
{
	return a < b or equality(a,b);
}

} //namespace libgs

[[nodiscard]] constexpr bool operator==
(libgs::concepts::arithmetic_p auto a, libgs::concepts::arithmetic_p auto b)
{
	if constexpr( libgs::is_float_v<decltype(a)> or libgs::is_float_v<decltype(b)> )
	{
		auto e = std::abs(a - b) ;
		return e < std::numeric_limits<decltype(e)>::epsilon();
	}
	else
		return a == b;
}

[[nodiscard]] constexpr bool operator!=
(libgs::concepts::arithmetic_p auto a, libgs::concepts::arithmetic_p auto b)
{
	return not libgs::equality(a,b);
}

[[nodiscard]] constexpr bool operator>=
(libgs::concepts::arithmetic_p auto a, libgs::concepts::arithmetic_p auto b)
{
	return a > b or libgs::equality(a,b);
}

[[nodiscard]] constexpr bool operator<=
(libgs::concepts::arithmetic_p auto a, libgs::concepts::arithmetic_p auto b)
{
	return a < b or libgs::equality(a,b);
}


#endif //LIBGS_CORE_CXX_DETAIL_OPERATOR_H
