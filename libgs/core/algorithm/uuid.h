
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

#ifndef LIBGS_CORE_ALGORITHM_UUID_H
#define LIBGS_CORE_ALGORITHM_UUID_H

#include <libgs/core/global.h>

namespace libgs
{

template <concepts::char_type CharT>
union LIBGS_CORE_TAPI basic_uuid // version 4
{
	static constexpr bool is_char_v = libgs::is_char_v<CharT>;

public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

public:
	basic_uuid(string_view_t basic_uuid);
	basic_uuid(const basic_uuid &other) = default;

public:
	[[nodiscard]] static basic_uuid generate();

public:
	basic_uuid &operator=(const basic_uuid &other) = default;
	basic_uuid &operator=(string_view_t basic_uuid);

public:
	bool operator==(const basic_uuid &other) const;
	bool operator!=(const basic_uuid &other) const;
	bool operator<(const basic_uuid &other) const;
	bool operator>(const basic_uuid &other) const;

public:
	// aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
	[[nodiscard]] string_t to_string(bool parcel = false) const;
	operator string_t() const { return to_string(); }

public:
	uint64_t wide_integers[2];

	struct internal_data
	{
		uint32_t d0;
		uint16_t d1;
		uint16_t d2;
		uint8_t  d3[8];
	}
	internals;

	struct byte_representation
	{
		uint8_t d0[4];
		uint8_t d1[2];
		uint8_t d2[2];
		uint8_t d3[8];
	}
	bytes;
};

using uuid = basic_uuid<char>;
using wuuid = basic_uuid<wchar_t>;

} //namespace libgs
#include <libgs/core/algorithm/detail/uuid.h>


#endif //LIBGS_CORE_ALGORITHM_UUID_H
