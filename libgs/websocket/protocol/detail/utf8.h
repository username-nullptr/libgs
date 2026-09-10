// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H
#define LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H

#include <cstdint>
#include <string_view>

namespace libgs::websocket::detail
{

class utf8_validator
{
	public:
	[[nodiscard]] bool consume(std::string_view text) noexcept
	{
		for(auto value : text)
		{
			const auto byte = static_cast<uint8_t>(value);
			if( m_remaining != 0 )
			{
				if( byte < m_lower or byte > m_upper )
					return false;
				--m_remaining;
				m_lower = 0x80;
				m_upper = 0xBF;
				continue;
			}

			if( byte <= 0x7F )
				continue;
			if( byte >= 0xC2 and byte <= 0xDF )
				m_remaining = 1;
			else if( byte == 0xE0 )
			{
				m_remaining = 2;
				m_lower = 0xA0;
			}
			else if( (byte >= 0xE1 and byte <= 0xEC) or
				(byte >= 0xEE and byte <= 0xEF) )
				m_remaining = 2;
			else if( byte == 0xED )
			{
				m_remaining = 2;
				m_upper = 0x9F;
			}
			else if( byte == 0xF0 )
			{
				m_remaining = 3;
				m_lower = 0x90;
			}
			else if( byte >= 0xF1 and byte <= 0xF3 )
				m_remaining = 3;
			else if( byte == 0xF4 )
			{
				m_remaining = 3;
				m_upper = 0x8F;
			}
			else
				return false;
		}
		return true;
	}

	[[nodiscard]] bool complete() const noexcept {
		return m_remaining == 0;
	}

	private:
	uint8_t m_remaining = 0;
	uint8_t m_lower = 0x80;
	uint8_t m_upper = 0xBF;
};

[[nodiscard]] inline bool is_valid_utf8(std::string_view text) noexcept
{
	utf8_validator validator;
	return validator.consume(text) and validator.complete();
}

} //namespace libgs::websocket::detail

#endif //LIBGS_WEBSOCKET_PROTOCOL_DETAIL_UTF8_H
