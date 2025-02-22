
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_DETAIL_OPERATOR_H
#define LIBGS_CORE_CXX_DETAIL_OPERATOR_H

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

bool equal_greater(concepts::number_type auto a, concepts::number_type auto b)
{
	return a > b or equality(a,b);
}

bool equal_less(concepts::number_type auto a, concepts::number_type auto b)
{
	return a < b or equality(a,b);
}

} //namespace libgs

[[nodiscard]] bool operator==(libgs::concepts::number_type auto a, libgs::concepts::number_type auto b)
{
	if constexpr( libgs::is_float_v<decltype(a)> or libgs::is_float_v<decltype(b)> )
	{
		auto e = std::abs(a - b) ;
		return e < std::numeric_limits<decltype(e)>::epsilon();
	}
	else
		return a == b;
}

[[nodiscard]] bool operator!=(libgs::concepts::number_type auto a, libgs::concepts::number_type auto b)
{
	return not libgs::equality(a,b);
}

[[nodiscard]] bool operator>=(libgs::concepts::number_type auto a, libgs::concepts::number_type auto b)
{
	return a > b or libgs::equality(a,b);
}

[[nodiscard]] bool operator<=(libgs::concepts::number_type auto a, libgs::concepts::number_type auto b)
{
	return a < b or libgs::equality(a,b);
}


#endif //LIBGS_CORE_CXX_DETAIL_OPERATOR_H