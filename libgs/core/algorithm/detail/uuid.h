
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

#include <cinttypes>
#include <random>
#include <format>
#include <cstdio>

namespace libgs
{

template <concept_char_type CharT>
basic_uuid<CharT>::basic_uuid(str_view_type basic_uuid) :
	_wide_integers{0}
{
	operator=(basic_uuid);
}

template <concept_char_type CharT>
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
	basic_uuid<CharT> obj {str_type()};

	obj._wide_integers[0] = dis64(gen);
	obj._wide_integers[1] = dis64(gen);

	obj._bytes.d3[0] = (obj._bytes.d3[0] & 0x3F) | static_cast<uint8_t>(0x80);
	obj._bytes.d2[1] = (obj._bytes.d2[1] & 0x0F) | static_cast<uint8_t>(0x40);
	return obj;
}

template <concept_char_type CharT>
basic_uuid<CharT> &basic_uuid<CharT>::operator=(str_view_type basic_uuid)
{
	if constexpr( is_char_v )
	{
		if( basic_uuid.size() == 38 ) //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
		{
			std::sscanf(basic_uuid.data(), "%08" SCNx32 "-%04" SCNx16 "-%04" SCNx16 "-%02" SCNx8 "%02" SCNx8 "-%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8,
						&_basic_uuid.d0, &_basic_uuid.d1, &_basic_uuid.d2, &_basic_uuid.d3[0], &_basic_uuid.d3[1], &_basic_uuid.d3[2], &_basic_uuid.d3[3], &_basic_uuid.d3[4], &_basic_uuid.d3[5], &_basic_uuid.d3[6], &_basic_uuid.d3[7]);
		}
		else if( basic_uuid.size() == 40 ) //{aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee}
		{
			std::sscanf(basic_uuid.data(), "{%08" SCNx32 "-%04" SCNx16 "-%04" SCNx16 "-%02" SCNx8 "%02" SCNx8 "-%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "}",
						&_basic_uuid.d0, &_basic_uuid.d1, &_basic_uuid.d2, &_basic_uuid.d3[0], &_basic_uuid.d3[1], &_basic_uuid.d3[2], &_basic_uuid.d3[3], &_basic_uuid.d3[4], &_basic_uuid.d3[5], &_basic_uuid.d3[6], &_basic_uuid.d3[7]);
		}
	}
	else
	{
		if( basic_uuid.size() == 38 ) //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
		{
			std::swscanf(basic_uuid.data(), L"%08" LIBGS_WCHAR(SCNx32) L"-%04" LIBGS_WCHAR(SCNx16) L"-%04" LIBGS_WCHAR(SCNx16) L"-%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"-%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8),
						 &_basic_uuid.d0, &_basic_uuid.d1, &_basic_uuid.d2, &_basic_uuid.d3[0], &_basic_uuid.d3[1], &_basic_uuid.d3[2], &_basic_uuid.d3[3], &_basic_uuid.d3[4], &_basic_uuid.d3[5], &_basic_uuid.d3[6], &_basic_uuid.d3[7]);
		}
		else if( basic_uuid.size() == 40 ) //{aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee}
		{
			std::swscanf(basic_uuid.data(), L"{%08" LIBGS_WCHAR(SCNx32) L"-%04" LIBGS_WCHAR(SCNx16) L"-%04" LIBGS_WCHAR(SCNx16) L"-%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"-%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"%02" LIBGS_WCHAR(SCNx8) L"}",
						 &_basic_uuid.d0, &_basic_uuid.d1, &_basic_uuid.d2, &_basic_uuid.d3[0], &_basic_uuid.d3[1], &_basic_uuid.d3[2], &_basic_uuid.d3[3], &_basic_uuid.d3[4], &_basic_uuid.d3[5], &_basic_uuid.d3[6], &_basic_uuid.d3[7]);
		}
	}
	return *this;
}

template <concept_char_type CharT>
bool basic_uuid<CharT>::operator==(const basic_uuid &other) const
{
	return std::memcmp(&other, this, sizeof(basic_uuid)) == 0;
}

template <concept_char_type CharT>
bool basic_uuid<CharT>::operator!=(const basic_uuid &other) const
{
	return not operator==(other);
}

template <concept_char_type CharT>
bool basic_uuid<CharT>::operator<(const basic_uuid &other) const
{
	return std::memcmp(this, &other, sizeof(basic_uuid)) < 0;
}

template <concept_char_type CharT>
bool basic_uuid<CharT>::operator>(const basic_uuid &other) const
{
	return std::memcmp(this, &other, sizeof(basic_uuid)) > 0;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_uuid<CharT>::to_string(bool parcel) const
{
	if constexpr( is_char_v )
	{
		char buffer[41] = ""; //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
		if( parcel )
		{
			std::sprintf(buffer, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
						 _basic_uuid.d0, _basic_uuid.d1, _basic_uuid.d2, _basic_uuid.d3[0], _basic_uuid.d3[1], _basic_uuid.d3[2], _basic_uuid.d3[3], _basic_uuid.d3[4], _basic_uuid.d3[5], _basic_uuid.d3[6], _basic_uuid.d3[7]);
		}
		else
		{
			std::sprintf(buffer, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
						 _basic_uuid.d0, _basic_uuid.d1, _basic_uuid.d2, _basic_uuid.d3[0], _basic_uuid.d3[1], _basic_uuid.d3[2], _basic_uuid.d3[3], _basic_uuid.d3[4], _basic_uuid.d3[5], _basic_uuid.d3[6], _basic_uuid.d3[7]);
		}
		return buffer;
	}
	else
	{
		wchar_t buffer[41] = L""; //aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
		if( parcel )
		{
			std::swprintf(buffer, 40, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
						  _basic_uuid.d0, _basic_uuid.d1, _basic_uuid.d2, _basic_uuid.d3[0], _basic_uuid.d3[1], _basic_uuid.d3[2], _basic_uuid.d3[3], _basic_uuid.d3[4], _basic_uuid.d3[5], _basic_uuid.d3[6], _basic_uuid.d3[7]);
		}
		else
		{
			std::swprintf(buffer, 40, L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
						  _basic_uuid.d0, _basic_uuid.d1, _basic_uuid.d2, _basic_uuid.d3[0], _basic_uuid.d3[1], _basic_uuid.d3[2], _basic_uuid.d3[3], _basic_uuid.d3[4], _basic_uuid.d3[5], _basic_uuid.d3[6], _basic_uuid.d3[7]);
		}
		return buffer;
	}
}

} //namespace libgs

namespace std
{

template <libgs::concept_char_type CharT>
class formatter<libgs::basic_uuid<CharT>, CharT> 
{
public:
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
