
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

#ifndef LIBGS_IO_TYPES_DETAIL_STREAM_BUFFER_H
#define LIBGS_IO_TYPES_DETAIL_STREAM_BUFFER_H

namespace libgs::io
{

inline buffer<void>::buffer(size_t size) :
	size(size)
{

}

inline buffer<void*>::buffer(void *data, size_t size) :
	buffer<void>(size), data(data)
{

}

inline buffer<std::string&>::buffer(std::string &data, size_t size) :
	buffer<void>(size), data(data)
{

}

inline buffer<std::string_view>::buffer(std::string_view data, size_t size)
{
	if( size == 0 )
	{
		this->data = data;
		this->size = data.size();
	}
	else
	{
		this->size = size > data.size() ? data.size() : size;
		this->data = std::string_view(data.data(), this->size);
	}
}

inline buffer<std::string_view>::buffer(const std::string &data, size_t size)
{
	if( size == 0 )
	{
		this->data = data;
		this->size = data.size();
	}
	else
	{
		this->size = size > data.size() ? data.size() : size;
		this->data = std::string_view(data.c_str(), this->size);
	}
}

inline buffer<std::string_view>::buffer(const void *data, size_t size) :
	buffer<void>(size), data(reinterpret_cast<const char*>(data), size)
{

}

inline buffer<std::string_view>::buffer(const char *data) :
	buffer<void>(strlen(data)), data(data)
{

}

} //namespace libgs::io


#endif //LIBGS_IO_TYPES_DETAIL_STREAM_BUFFER_H
