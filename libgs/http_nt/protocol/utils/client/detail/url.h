
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_DETAIL_URL_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_DETAIL_URL_H

namespace libgs::http_nt
{

template <typename Arg0, typename...Args>
url::url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args) :
	url(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <typename Arg0, typename...Args>
url &url::emplace(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args)
{
	set(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...));
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_DETAIL_URL_H
