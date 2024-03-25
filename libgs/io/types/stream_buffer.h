
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

#ifndef LIBGS_IO_STREAM_BUFFER_H
#define LIBGS_IO_STREAM_BUFFER_H

#include <libgs/io/global.h>

namespace libgs
{

template <typename T>
struct buffer;

template <>
struct LIBGS_IO_API buffer<void>
{
	size_t size;

protected:
	buffer(size_t size = 0);
};

template <>
struct LIBGS_IO_API buffer<void*> : buffer<void>
{
	void *data;
	buffer(void *data, size_t size = 0);
};

template <>
struct LIBGS_IO_API buffer<std::string&> : buffer<void>
{
	std::string &data;
	buffer(std::string &data, size_t size = 0);
};

template <>
struct LIBGS_IO_API buffer<const void*> : buffer<void>
{
	const void *data;
	buffer(const void *data, size_t size = 0);
};

template <>
struct LIBGS_IO_API buffer<const std::string&> : buffer<void>
{
	using this_type = buffer<const std::string&>;
	const std::string &data;

	template <typename String>
	buffer(const String &data, size_t size = 0) requires std::is_same_v<String,std::string>;
};

} //namespace libgs
#include <libgs/io/types/detail/stream_buffer.h>


#endif //LIBGS_IO_STREAM_BUFFER_H
