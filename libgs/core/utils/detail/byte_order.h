
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_UTILS_DETAIL_BYTE_ORDER_H
#define LIBGS_CORE_UTILS_DETAIL_BYTE_ORDER_H

namespace libgs
{

inline bool is_little_endian()
{
	static constexpr uint32_t i = 0x12345678;
	return *reinterpret_cast<const char*>(&i) == 0x78;
}

inline bool is_big_endian()
{
	return not is_little_endian();
}

auto hton(concepts::arithmetic_p auto t)
{
	return is_big_endian() ? t : reverse(t);
}

auto hton(concepts::enumerate_p auto e)
{
	using type = byte_type<sizeof(e)>::unsigned_t;
	using enum_t = std::remove_cvref_t<decltype(e)>;
	return static_cast<enum_t>(hton(static_cast<type>(e)));
}

auto *hton(auto *data, size_t len)
{
	return is_big_endian() ? data : reverse(data, len);
}

auto ntoh(concepts::arithmetic_p auto t)
{
	return is_big_endian() ? t : reverse(t);
}

auto ntoh(concepts::enumerate_p auto e)
{
	using type = byte_type<sizeof(e)>::unsigned_t;
	using enum_t = std::remove_cvref_t<decltype(e)>;
	return static_cast<enum_t>(ntoh(static_cast<type>(e)));
}

auto *ntoh(auto *data, size_t len)
{
	return is_big_endian() ? data : reverse(data, len);
}

auto reverse(concepts::arithmetic_p auto t)
{
	for(size_t i=0; i<sizeof(t)>>1; i++)
	{
		auto m = reinterpret_cast<char*>(&t) + i;
		auto n = reinterpret_cast<char*>(&t) + sizeof(t) - i - 1;
		std::swap(*m, *n);
	}
	return t;
}

auto reverse(concepts::enumerate_p auto e)
{
	using type = byte_type<sizeof(e)>::unsigned_t;
	using enum_t = std::remove_cvref_t<decltype(e)>;
	return static_cast<enum_t>(reverse(static_cast<type>(e)));
}

auto *reverse(auto *data, size_t len)
{
	constexpr auto type_len = sizeof(*data);
	for(size_t i=0; i<len; i++)
	{
		auto array = reinterpret_cast<char*>(data + i);
		for(size_t j=0; j<type_len>>1; j++)
			std::swap(array[j], array[type_len - j - 1]);
	}
	return data;
}

auto to_big_endian(concepts::arithmetic_p auto t)
{
	return is_big_endian() ? t : reverse(t);
}

auto to_big_endian(concepts::enumerate_p auto e)
{
	using type = byte_type<sizeof(e)>::unsigned_t;
	using enum_t = std::remove_cvref_t<decltype(e)>;
	return static_cast<enum_t>(to_big_endian(static_cast<type>(e)));
}

auto *to_big_endian(auto *data, size_t len)
{
	return is_big_endian() ? data : reverse(data, len);
}

auto to_little_endian(concepts::arithmetic_p auto t)
{
	return is_little_endian() ? t : reverse(t);
}

auto to_little_endian(concepts::enumerate_p auto e)
{
	using type = byte_type<sizeof(e)>::unsigned_t;
	using enum_t = std::remove_cvref_t<decltype(e)>;
	return static_cast<enum_t>(to_little_endian(static_cast<type>(e)));
}

auto *to_little_endian(auto *data, size_t len)
{
	return is_little_endian() ? data : reverse(data, len);
}

} //namespace libgs


#endif //LIBGS_CORE_UTILS_DETAIL_BYTE_ORDER_H

