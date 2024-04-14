
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

#ifndef LIBGS_IO_DETAIL_STREAM_BUFFER_H
#define LIBGS_IO_DETAIL_STREAM_BUFFER_H

namespace libgs::io
{

template <typename String>
buffer<const std::string&>::buffer(const String &data, size_t size) requires std::is_same_v<String,std::string> :
	buffer<void>(size), data(data)
{
	if( size == 0 )
		this->size = data.size();
}

template <typename Args0, typename...Args>
buffer<const std::string&>::buffer(std::format_string<Args...> fmt, Args0 &&arg0, Args&&...args)
{
	data = std::format(fmt, std::forward<Args0>(arg0), std::forward<Args>(args)...);
	size = data.size();
}

template <typename Args0, typename...Args>
buffer<const std::string&>::buffer(std::wformat_string<Args...> fmt, Args0 &&arg0, Args&&...args)
{
	data = std::format(fmt, std::forward<Args0>(arg0), std::forward<Args>(args)...);
	size = data.size();
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_STREAM_BUFFER_H
