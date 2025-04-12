
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_UUID_H
#define LIBGS_CORE_ALGORITHM_DETAIL_UUID_H

#include <cinttypes>
#include <random>
#include <cuchar>
#include <cstdio>

namespace libgs
{

template <concepts::character CharT>
basic_uuid<CharT>::basic_uuid(string_view_t basic_uuid)
{
	operator=(basic_uuid);
}

template <concepts::character CharT>
basic_uuid<CharT> basic_uuid<CharT>::generate()
{
#if defined(__APPLE__) || defined(__clang__)
	std::random_device rd;
	std::mt19937_64 gen(rd());
#else
	thread_local std::random_device rd;
	thread_local std::mt19937_64 gen(rd());
#endif
	std::uniform_int_distribution<uint64_t> dis64;
	basic_uuid obj {string_t()};

	obj.wide_integers[0] = dis64(gen);
	obj.wide_integers[1] = dis64(gen);

	obj.bytes.d3[0] = (obj.bytes.d3[0] & 0x3F) | static_cast<uint8_t>(0x80);
	obj.bytes.d2[1] = (obj.bytes.d2[1] & 0x0F) | static_cast<uint8_t>(0x40);
	return obj;
}

template <concepts::character CharT>
basic_uuid<CharT> &basic_uuid<CharT>::operator=(string_view_t basic_uuid)
{
	do {
		size_t star_idx = 0;
		size_t idxes[4] = { 8, 13, 18, 23 };
		if( basic_uuid.size() == 40 ) //{aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee}
		{
			if( basic_uuid[0] != static_cast<char_t>('{') or basic_uuid[39] != static_cast<char_t>('}') )
				break;
			star_idx = 1;
			for(auto &idx : idxes)
				idx += 1;
		}
		else if( basic_uuid.size() != 38 ) //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
			break;
		for(auto &idx : idxes)
		{
			if( basic_uuid[idx] != static_cast<char_t>('-') )
				break;
		}
		try {
			internals.d0 = strtls::to_uint32(basic_uuid.substr(star_idx,8), 16);
			star_idx += 8 + 1;

			internals.d1 = strtls::to_uint16(basic_uuid.substr(star_idx,4), 16);
			star_idx += 4 + 1;
			internals.d2 = strtls::to_uint16(basic_uuid.substr(star_idx,4), 16);
			star_idx += 4 + 1;

			for(size_t i=0; i<8; i++)
			{
				internals.d3[i] = strtls::to_uint8(basic_uuid.substr(star_idx,2), 16);
				star_idx += 2 + 1;
			}
		}
		catch(...) {
			break;
		}
		return *this;
	}
	while(0);
	std::memset(this, 0, sizeof(*this));
	return *this;
}

template <concepts::character CharT>
bool basic_uuid<CharT>::operator==(const basic_uuid &other) const
{
	return std::memcmp(&other, this, sizeof(basic_uuid)) == 0;
}

template <concepts::character CharT>
bool basic_uuid<CharT>::operator!=(const basic_uuid &other) const
{
	return not operator==(other);
}

template <concepts::character CharT>
bool basic_uuid<CharT>::operator<(const basic_uuid &other) const
{
	return std::memcmp(this, &other, sizeof(basic_uuid)) < 0;
}

template <concepts::character CharT>
bool basic_uuid<CharT>::operator>(const basic_uuid &other) const
{
	return std::memcmp(this, &other, sizeof(basic_uuid)) > 0;
}

template <concepts::character CharT>
std::basic_string<CharT> basic_uuid<CharT>::to_string(bool parcel) const
{
	char buffer[41] = ""; //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
	if( parcel )
	{
		std::snprintf(buffer, 40, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			internals.d0, internals.d1, internals.d2, internals.d3[0], internals.d3[1], internals.d3[2],
			internals.d3[3], internals.d3[4], internals.d3[5], internals.d3[6], internals.d3[7]
		);
	}
	else
	{
		std::snprintf(buffer, 40, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			internals.d0, internals.d1, internals.d2, internals.d3[0], internals.d3[1], internals.d3[2],
			internals.d3[3], internals.d3[4], internals.d3[5], internals.d3[6], internals.d3[7]
		);
	}
	return strtls::detail::ascii_transition<char_t>(buffer);
}

} //namespace libgs

namespace std
{

template <libgs::concepts::character CharT>
struct formatter<libgs::basic_uuid<CharT>, CharT>
{
	auto format(const libgs::basic_uuid<CharT> &uuid, auto &context) const {
		return m_formatter.format(uuid.to_string(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};

} //namespace std


#endif //LIBGS_CORE_ALGORITHM_DETAIL_UUID_H
