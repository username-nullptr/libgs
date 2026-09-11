// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/protocol/utils/server/parser.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if( size < 2 )
		return 0;

	libgs::http::server_parser parser((data[0] % 64) + 1);
	const auto chunk_size = size_t(data[1] % 127) + 1;

	size_t offset = 2;
	while( offset < size )
	{
		const auto available = std::min(chunk_size, size - offset);
		auto parsed = parser.append(libgs::const_buffer(data + offset, available));
		offset += available;
		if( not parsed or parser.stage() == libgs::http::stage::finished )
			break;
	}

	if( parser.stage() != libgs::http::stage::header )
	{
		libgs::ignore_unused(parser.method());
		libgs::ignore_unused(parser.target_form());
		libgs::ignore_unused(parser.target());
		libgs::ignore_unused(parser.path());
		libgs::ignore_unused(parser.version());
		libgs::ignore_unused(parser.keep_alive());
		libgs::ignore_unused(parser.support_gzip());
		libgs::ignore_unused(parser.take_body());
		libgs::ignore_unused(parser.take_pending_data());
	}
	return 0;
}
