
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

template <concept_char_type CharT>
union basic_uuid // version 4
{
	using str_type = std::basic_string<CharT>;
	static constexpr bool is_char_v = libgs::is_char_v<CharT>;

public:
	basic_uuid(const str_type &basic_uuid);
	basic_uuid(const basic_uuid &other) = default;

public:
	[[nodiscard]] static basic_uuid generate();

public:
	basic_uuid &operator=(const basic_uuid &other) = default;
	basic_uuid &operator=(const str_type &basic_uuid);

public:
	bool operator==(const basic_uuid &other) const;
	bool operator!=(const basic_uuid &other) const;
	bool operator<(const basic_uuid &other) const;
	bool operator>(const basic_uuid &other) const;

public:
	// aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
	[[nodiscard]] str_type to_string(bool parcel = false) const;
	operator str_type() const { return to_string(); }

public:
	uint64_t _wide_integers[2];

	struct _internalData
	{
		uint32_t d0;
		uint16_t d1;
		uint16_t d2;
		uint8_t  d3[8];
	}
	_basic_uuid;

	struct _byteRepresentation
	{
		uint8_t d0[4];
		uint8_t d1[2];
		uint8_t d2[2];
		uint8_t d3[8];
	}
	_bytes;
};

using uuid = basic_uuid<char>;

using wuuid = basic_uuid<wchar_t>;

} //namespace libgs
#include <libgs/core/algorithm/detail/uuid.h>


#endif //LIBGS_CORE_ALGORITHM_UUID_H
