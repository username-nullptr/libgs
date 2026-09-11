// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/client.h>

namespace libgs::websocket::detail
{

bool ascii_equal_case_insensitive(std::string_view lhs, std::string_view rhs) noexcept
{
	if( lhs.size() != rhs.size() )
		return false;

	for(size_t index = 0; index < lhs.size(); ++index)
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

bool websocket_owned_request_header(std::string_view name) noexcept
{
	return ascii_equal_case_insensitive(name, http::header::host) or
		   ascii_equal_case_insensitive(name, http::header::connection) or
		   ascii_equal_case_insensitive(name, http::header::upgrade) or
		   ascii_equal_case_insensitive(name, http::header::proxy_authorization) or
		   (name.size() >= 14 and ascii_equal_case_insensitive(name.substr(0, 14), "Sec-WebSocket-"));
}

sys_expected<url> canonical_websocket_url(const url &endpoint) noexcept
{
	try {
		if( not endpoint.is_valid() or endpoint.host().empty() or endpoint.has_fragment())
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		const auto scheme = strtls::to_lower(endpoint.protocol());
		std::string_view canonical_scheme;

		if(scheme == "ws" or scheme == "http")
			canonical_scheme = "ws";

		else if(scheme == "wss" or scheme == "https")
			canonical_scheme = "wss";
		else
		{
			return sys_unexpected (
				make_error_code(std::errc::protocol_not_supported)
			);
		}
		auto text = endpoint.to_string();
		auto separator = text.find("://");

		if( separator == std::string::npos )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		text.replace(0, separator, canonical_scheme);
		url result(text);

		if( not result.is_valid() )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		return result;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

error_code validate_open_request(connect_request &request, const stream_config &stream) noexcept
{
	try {
		auto endpoint = canonical_websocket_url(request.endpoint);
		if( not endpoint )
			return endpoint.error();

		request.endpoint = std::move(*endpoint);
		if( stream.read_buffer_size == 0 )
			return make_error_code(std::errc::invalid_argument);

		if( not request.extensions.empty() )
			return make_error_code(errc::unsupported_extension);

		for(const auto &[name, value] : request.request_options.headers())
		{
			ignore_unused(value);
			if( websocket_owned_request_header(name) )
				return make_error_code(errc::invalid_upgrade);
		}
		return {};
	}
	catch(const std::bad_alloc&) {
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {}
	return make_error_code(std::errc::io_error);
}

sys_expected<url> http_transport_url(const url &endpoint) noexcept
{
	try {
		auto text = endpoint.to_string();
		auto separator = text.find("://");

		if( separator == std::string::npos )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		const auto scheme = strtls::to_lower(endpoint.protocol());
		text.replace(0, separator, scheme == "wss" ? "https" : "http");

		url result(text);
		if( not result.is_valid() )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		return result;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

uint16_t websocket_effective_port(const url &value) noexcept
{
	if( value.port() != 0 )
		return value.port();
	return ascii_equal_case_insensitive(value.protocol(), "wss") ? 443 : 80;
}

bool same_websocket_origin(const url &lhs, const url &rhs) noexcept
{
	return ascii_equal_case_insensitive(lhs.protocol(), rhs.protocol()) and
		   ascii_equal_case_insensitive(lhs.host(), rhs.host()) and
		   websocket_effective_port(lhs) == websocket_effective_port(rhs);
}

bool websocket_redirect_status(http::status_enum status) noexcept
{
	return status == http::status::moved_permanently or
		   status == http::status::found or
		   status == http::status::see_other or
		   status == http::status::temporary_redirect or
		   status == http::status::permanent_redirect;
}

std::vector<std::byte> pending_bytes(std::string pending)
{
	std::vector<std::byte> result(pending.size());
	if( not pending.empty() )
		std::memcpy(result.data(), pending.data(), pending.size());
	return result;
}

std::chrono::milliseconds remaining_timeout(std::chrono::steady_clock::time_point deadline) noexcept
{
	const auto now = std::chrono::steady_clock::now();
	if( now >= deadline )
		return std::chrono::milliseconds::zero();

	auto remaining = std::chrono::duration_cast
		<std::chrono::milliseconds>(deadline - now);

	return remaining > std::chrono::milliseconds::zero() ?
		remaining : std::chrono::milliseconds(1);
}

} //namespace libgs::websocket::detail
