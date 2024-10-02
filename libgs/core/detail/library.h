
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

#ifndef LIBGS_CORE_DETAIL_LIBRARY_H
#define LIBGS_CORE_DETAIL_LIBRARY_H

namespace libgs
{

namespace detail
{

template <typename T>
struct tuple_reverse;

template <typename...Args>
struct tuple_reverse<std::tuple<Args...>>
{

};

} //namespace detail

template <concepts::function Func>
auto library::interface(std::string_view ifname) const
{
	using function_t = std::function<typename function_traits<Func>::call_type>;
	using pointer_t = typename function_traits<Func>::pointer_type;
	return function_t(reinterpret_cast<pointer_t>(interface(ifname)));
}

template <concepts::function Func, typename Arg0, typename...Args>
auto library::interface(std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args) const
{
	return interface<Func>(std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...));
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LIBRARY_H
