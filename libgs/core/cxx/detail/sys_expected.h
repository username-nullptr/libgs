
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_CXX_DETAIL_SYS_EXPECTED_H
#define LIBGS_CORE_CXX_DETAIL_SYS_EXPECTED_H

namespace libgs
{

template <concepts::optional_value_p Value>
auto make_sys_expected(Value &&value)
{
	return make_expected<error_code>(std::forward<Value>(value));
}

inline sys_expected<> make_sys_expected()
{
	return make_expected<error_code>();
}

template <concepts::expected_value Value>
void sys_expected_loc_throw(const sys_expected<Value> &expected, std::source_location loc)
{
	if( not expected )
		system_error::loc_throw(expected.error(), std::move(loc));
}

template <concepts::expected_value Value>
void sys_expected_loc_throw
(const sys_expected<Value> &expected, concepts::text_p<char> auto &&msg, std::source_location loc)
{
	if( not expected )
		system_error::loc_throw(expected.error(), strtls::to_view(msg), std::move(loc));
}

inline io_expected make_io_expected(size_t sum)
{
	return make_sys_expected(sum);
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_SYS_EXPECTED_H
