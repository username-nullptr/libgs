
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

#ifndef LIBGS_CORE_DETAIL_MODULES_H
#define LIBGS_CORE_DETAIL_MODULES_H

namespace libgs
{

void modules::reg_init(concepts::modules_init_func auto &&func, level_t level)
{
	using Func = std::decay_t<decltype(func)>;
	using return_t = typename function_traits<Func>::return_type;

	if constexpr( std::is_same_v<return_t,void> )
		reg_init_p(init_func_t(std::forward<Func>(func)), level);

	else if constexpr( std::is_same_v<return_t,std::future<void>> )
		reg_init_p(future_init_func_t(std::forward<Func>(func)), level);

	else if constexpr( std::is_same_v<return_t,awaitable<void>> )
		reg_init_p(await_init_func_t(std::forward<Func>(func)), level);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_MODULES_H