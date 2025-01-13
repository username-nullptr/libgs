
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

#ifndef LIBGS_CORE_CXX_DETAIL_TYPE_TRAITS_H
#define LIBGS_CORE_CXX_DETAIL_TYPE_TRAITS_H

namespace libgs
{

inline const_buffer::const_buffer(const asio::ASIO_CONST_BUFFER &buf) :
	asio::ASIO_CONST_BUFFER(buf.data(), buf.size())
{

}

inline const_buffer::const_buffer(const mutable_buffer &buf) :
	asio::ASIO_CONST_BUFFER(buf.data(), buf.size())
{

}

inline const_buffer::const_buffer(const char *buf) :
	asio::ASIO_CONST_BUFFER(buf, strlen(buf))
{

}

inline const_buffer::const_buffer(const std::string &buf) :
	asio::ASIO_CONST_BUFFER(buf.c_str(), buf.size())
{

}

inline const_buffer::const_buffer(std::string_view buf) :
	asio::ASIO_CONST_BUFFER(buf.data(), buf.size())
{

}

inline const_buffer &const_buffer::operator=(const mutable_buffer &buf)
{
	operator=(const_buffer(buf.data(), buf.size()));
	return *this;
}

template <typename...Args>
auto buffer(Args&&...args)
{
	return asio::buffer(std::forward<Args>(args)...);
}

} //namespace libgs

#endif //LIBGS_CORE_CXX_DETAIL_TYPE_TRAITS_H