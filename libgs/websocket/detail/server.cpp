// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/server.h>

namespace libgs::websocket::detail
{

bool server_ascii_equal_case_insensitive(std::string_view lhs, std::string_view rhs) noexcept
{
	if( lhs.size() != rhs.size() )
		return false;

	for(size_t index=0; index<lhs.size(); ++index)
	{
		auto left = static_cast<unsigned char>(lhs[index]);
		auto right = static_cast<unsigned char>(rhs[index]);

		if( left >= 'A' and left <= 'Z' )
			left = static_cast<unsigned char>(left + ('a' - 'A'));

		if( right >= 'A' and right <= 'Z' )
			right = static_cast<unsigned char>(right + ('a' - 'A'));

		if( left != right )
			return false;
	}
	return true;
}

bool server_deadline_expired(std::chrono::steady_clock::time_point deadline) noexcept
{
	return std::chrono::steady_clock::now() >= deadline;
}

std::chrono::milliseconds server_remaining_timeout
(std::chrono::steady_clock::time_point deadline) noexcept
{
	const auto now = std::chrono::steady_clock::now();
	if( now >= deadline )
		return std::chrono::milliseconds::zero();

	auto remaining = std::chrono::duration_cast
		<std::chrono::milliseconds>(deadline - now);

	return remaining > std::chrono::milliseconds::zero() ?
		remaining : std::chrono::milliseconds(1);
}

bool valid_rejection_status(http::status_enum status) noexcept
{
	const auto value = static_cast<uint32_t>(status);
	return value >= 200 and value <= 599 and
		status != http::status::switching_protocols;
}

void erase_protocol_response_headers(http::headers &headers) noexcept
{
	headers.erase(http::header::connection);
	headers.erase(http::header::upgrade);

	headers.erase(sec_websocket_accept);
	headers.erase(sec_websocket_version);

	headers.erase(sec_websocket_protocol);
	headers.erase(sec_websocket_extensions);

	headers.erase(http::header::content_length);
	headers.erase(http::header::transfer_encoding);
}

} //namespace libgs::websocket::detail
