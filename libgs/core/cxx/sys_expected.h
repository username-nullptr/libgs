
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

#ifndef LIBGS_CORE_CXX_SYS_EXPECTED_H
#define LIBGS_CORE_CXX_SYS_EXPECTED_H

#include <libgs/core/cxx/expected.h>

namespace libgs
{

template <concepts::expected_value Value = void>
using sys_expected = expected<Value,error_code>;

using sys_unexpected = unexpected<error_code>;

template <concepts::optional_value_p Value>
[[nodiscard]] LIBGS_CORE_TAPI auto make_sys_expected(Value &&value);
[[nodiscard]] LIBGS_CORE_VAPI sys_expected<> make_sys_expected();

template <concepts::expected_value Value = void>
LIBGS_CORE_VAPI void sys_expected_loc_throw(const sys_expected<Value> &expected,
    std::source_location loc = std::source_location::current()
);

using io_expected = sys_expected<size_t>;
using io_unexpected = sys_unexpected;

[[nodiscard]] LIBGS_CORE_TAPI io_expected make_io_expected(size_t sum);

} //namespace libgs
#include <libgs/core/cxx/detail/sys_expected.h>


#endif //LIBGS_CORE_CXX_SYS_EXPECTED_H