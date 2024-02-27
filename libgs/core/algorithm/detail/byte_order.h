
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_BYTE_ORDER_H
#define LIBGS_CORE_ALGORITHM_DETAIL_BYTE_ORDER_H

namespace libgs
{

inline bool is_big_endian()
{
	return not is_little_endian();
}

auto hton(concept_number_type auto t)
{
	return is_big_endian() ? t : reverse(t);
}

auto *hton(auto *data, size_t len)
{
	return is_big_endian() ? data : reverse(data, len);
}

auto ntoh(concept_number_type auto t)
{
	return is_big_endian() ? t : reverse(t);
}

auto *ntoh(auto *data, size_t len)
{
	return is_big_endian() ? data : reverse(data, len);
}

auto reverse(concept_number_type auto t)
{
	for(size_t i=0; i<sizeof(t)>>1; i++)
	{
		auto m = reinterpret_cast<char*>(&t) + i;
		auto n = reinterpret_cast<char*>(&t) + sizeof(t) - i - 1;

		*m ^= *n;
		*n ^= *m;
		*m ^= *n;
	}
	return t;
}

auto *reverse(auto *data, size_t len)
{
	auto array = reinterpret_cast<char*>(data);
	for(size_t i=0; i<len>>1; i++)
	{
		auto &m = array[i];
		auto &n = array[len - i - 1];

		m ^= n;
		n ^= m;
		m ^= n;
	}
	return data;
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_BYTE_ORDER_H

