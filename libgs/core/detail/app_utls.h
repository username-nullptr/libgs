
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

#ifndef LIBGS_CORE_DETAIL_APPLICATION_H
#define LIBGS_CORE_DETAIL_APPLICATION_H

namespace libgs::app
{

template <typename Arg0, typename...Args>
bool setenv(const std::string &key, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args)
{
	return setenv(key, std::format(fmt_value, std::forward<Args>(args)...));
}

template <typename Arg0, typename...Args>
bool setenv(const std::string &key, bool overwrite, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args)
{
	return setenv(key, std::format(fmt_value, std::forward<Args>(args)...), overwrite);
}

template <concept_char_string_type T>
bool setenv(const std::string &key, T &&value, bool overwrite)
{
	return setenv(key, std::format("{}", std::forward<T>(value)), overwrite);
}

} //namespace libgs::app


#endif //LIBGS_CORE_DETAIL_APPLICATION_H
