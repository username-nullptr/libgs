// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/protocol/parser.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if( size < 3 )
		return 0;

	libgs::websocket::frame_codec_config config;
	config.local_role = (data[0] & 1) ?
		libgs::websocket::role::client : libgs::websocket::role::server;
	config.max_frame_size = (data[1] & 1) ? 0 : 1024 * 1024;
	config.allowed_rsv = libgs::websocket::reserved_bits(
		static_cast<libgs::websocket::reserved_bit>(data[1] & 0x07));
	libgs::websocket::frame_parser parser(config);

	std::vector<std::byte> input(size - 2);
	std::memcpy(input.data(), data + 2, input.size());
	const auto chunk_size = size_t(data[2] % 31) + 1;
	size_t offset = 0;
	while( offset < input.size() )
	{
		const auto available = std::min(chunk_size, input.size() - offset);
		auto parsed = parser.parse(libgs::mutable_buffer(
			input.data() + offset, available));
		if( not parsed or parsed->consumed == 0 )
			break;
		offset += parsed->consumed;
	}
	return 0;
}
