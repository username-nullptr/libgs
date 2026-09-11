// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/protocol/utils/client/parser.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if( size < 3 )
		return 0;

	static constexpr std::array methods {
		libgs::http::method::get,
		libgs::http::method::head,
		libgs::http::method::connect,
	};
	libgs::http::client_parser parser((data[0] % 64) + 1);
	parser.set_request_method(methods[data[1] % methods.size()]);
	parser.set_automatic_decompression((data[1] & 0x80) != 0);
	const auto chunk_size = size_t(data[2] % 127) + 1;

	size_t offset = 3;
	bool failed = false;
	while( offset < size )
	{
		const auto available = std::min(chunk_size, size - offset);
		auto parsed = parser.append(libgs::const_buffer(data + offset, available));
		offset += available;
		if( not parsed )
		{
			failed = true;
			break;
		}
		if( parser.stage() == libgs::http::stage::finished )
			break;
	}
	if( not failed and parser.stage() != libgs::http::stage::header )
		libgs::ignore_unused(parser.finish_eof());

	if( parser.stage() != libgs::http::stage::header )
	{
		libgs::ignore_unused(parser.version());
		libgs::ignore_unused(parser.status());
		libgs::ignore_unused(parser.keep_alive());
		libgs::ignore_unused(parser.is_chunked());
		libgs::ignore_unused(parser.is_range_response());
		libgs::ignore_unused(parser.is_multipart_byte_ranges());
		libgs::ignore_unused(parser.is_informational());
		libgs::ignore_unused(parser.is_upgrade());
		libgs::ignore_unused(parser.take_body());
		libgs::ignore_unused(parser.take_range_body(1024));
		libgs::ignore_unused(parser.take_pending_data());
	}
	return 0;
}
