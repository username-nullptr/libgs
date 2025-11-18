
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

#ifndef LIBGS_CORE_CXX_SYSTEM_ERROR_H
#define LIBGS_CORE_CXX_SYSTEM_ERROR_H

#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/concepts.h>
#include <system_error>

namespace libgs
{

class LIBGS_CORE_VAPI error_code : public std::error_code
{
public:
	using std::error_code::error_code;
	using std::error_code::operator=;

	error_code(const std::error_code &error);
	error_code(std::error_code &&error);

	const error_code &exception(const std::string &what = "") const;
	error_code &exception(const std::string &what = "");

public:
	template <typename Func>
	static constexpr bool and_then_v =
		concepts::callable_ret<Func,error_code> or
		concepts::callable_void<Func>;

	template <typename Func>
	auto and_then(Func &&func) requires and_then_v<Func>;

	template <typename Func>
	static constexpr bool or_else_v =
		concepts::callable_ret<Func,error_code,error_code> or
		concepts::callable_void<Func,error_code> or
		concepts::callable_ret<Func,error_code> or
		concepts::callable_void<Func>;

	template <typename Func>
	error_code or_else(Func &&func) requires or_else_v<Func>;
};

} //namespace libgs
#include <libgs/core/cxx/detail/system_error.h>


#endif //LIBGS_CORE_CXX_SYSTEM_ERROR_H