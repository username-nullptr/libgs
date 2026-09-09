// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H
#define LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H

#include <cstdint>
#include <string_view>

namespace libgs::websocket::detail
{

[[nodiscard]] inline bool is_valid_utf8(std::string_view text) noexcept
{
	const auto *data = reinterpret_cast<const uint8_t*>(text.data());
	const auto size = text.size();
	size_t index = 0;

	const auto continuation = [](uint8_t value) noexcept {
		return value >= 0x80 and value <= 0xBF;
	};

	while( index < size )
	{
		const auto first = data[index];
		if( first <= 0x7F )
		{
			index++;
			continue;
		}

		if( first >= 0xC2 and first <= 0xDF )
		{
			if( size - index < 2 or not continuation(data[index + 1]) )
				return false;
			index += 2;
			continue;
		}

		if( first == 0xE0 )
		{
			if( size - index < 3 or data[index + 1] < 0xA0 or
				data[index + 1] > 0xBF or not continuation(data[index + 2]) )
				return false;
			index += 3;
			continue;
		}

		if( (first >= 0xE1 and first <= 0xEC) or
			(first >= 0xEE and first <= 0xEF) )
		{
			if( size - index < 3 or not continuation(data[index + 1]) or
				not continuation(data[index + 2]) )
				return false;
			index += 3;
			continue;
		}

		if( first == 0xED )
		{
			if( size - index < 3 or data[index + 1] < 0x80 or
				data[index + 1] > 0x9F or not continuation(data[index + 2]) )
				return false;
			index += 3;
			continue;
		}

		if( first == 0xF0 )
		{
			if( size - index < 4 or data[index + 1] < 0x90 or
				data[index + 1] > 0xBF or not continuation(data[index + 2]) or
				not continuation(data[index + 3]) )
				return false;
			index += 4;
			continue;
		}

		if( first >= 0xF1 and first <= 0xF3 )
		{
			if( size - index < 4 or not continuation(data[index + 1]) or
				not continuation(data[index + 2]) or
				not continuation(data[index + 3]) )
				return false;
			index += 4;
			continue;
		}

		if( first == 0xF4 )
		{
			if( size - index < 4 or data[index + 1] < 0x80 or
				data[index + 1] > 0x8F or not continuation(data[index + 2]) or
				not continuation(data[index + 3]) )
				return false;
			index += 4;
			continue;
		}

		return false;
	}
	return true;
}

} //namespace libgs::websocket::detail

#endif //LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H
