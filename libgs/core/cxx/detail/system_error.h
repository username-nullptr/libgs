
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

#ifndef LIBGS_CORE_CXX_DETAIL_SYSTEM_ERROR_H
#define LIBGS_CORE_CXX_DETAIL_SYSTEM_ERROR_H

namespace libgs
{

inline error_code::error_code(const std::error_code &error) :
	std::error_code(error)
{

}

inline error_code::error_code(std::error_code &&error) :
	std::error_code(std::move(error))
{

}

inline const error_code &error_code::exception(const std::string &what) const
{
	if( not *this )
		throw std::system_error(*this, what);
	return *this;
}

inline error_code &error_code::exception(const std::string &what)
{
	if( not *this )
		throw std::system_error(*this, what);
	return *this;
}

template <typename Func>
auto error_code::and_then(Func &&func) requires and_then_v<Func>
{
	if constexpr( concepts::callable_ret<Func,error_code> )
		return operator bool() ? *this : func();

	else if constexpr( concepts::callable_void<Func> )
	{
		if( not *this )
			func();
		return *this;
	}
}

template <typename Func>
error_code error_code::or_else(Func &&func) requires or_else_v<Func>
{
	if constexpr( concepts::callable_ret<Func,error_code,error_code> )
		return operator bool() ? func(*this) : *this;

	else if constexpr( concepts::callable_ret<Func,error_code> )
		return operator bool() ? func() : *this;

	else if constexpr( concepts::callable_void<Func,error_code> )
	{
		if( *this )
			func(*this);
		return *this;
	}
	else if constexpr( concepts::callable_void<Func> )
	{
		if( *this )
			func();
		return *this;
	}
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_SYSTEM_ERROR_H