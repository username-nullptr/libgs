// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/ctrl_payload.h>
#include <libgs/websocket/protocol/generator.h>
#include <libgs/websocket/protocol/parser.h>
#include <libgs/websocket/protocol/types.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size == 0 or size > 4'096)
		return 0;

	const std::string input(reinterpret_cast<const char*>(data), size);
	std::vector<std::byte> bytes(size);
	std::memcpy(bytes.data(), data, size);
	libgs::websocket::ctrl_payload control(bytes);
	libgs::ignore_unused(control.text());
	libgs::ignore_unused(control.as_mutable_buffer());
	libgs::ignore_unused(control.as_const_buffer());
	control.resize(data[0] % 126);
	const auto assigned = std::string_view(input).substr(0, data[0] % size);
	control.assign(assigned);
	if(control.text() != assigned or control.size() != assigned.size())
		std::abort();
	if((data[0] & 1U) != 0)
		control.clear();

	libgs::websocket::masking_key key;
	for(size_t index = 0; index < key.bytes.size(); ++index)
		key.bytes[index] = std::byte {data[index % size]};
	libgs::websocket::frame_header header;
	header.fin = (data[0] & 1U) != 0;
	header.rsv = libgs::websocket::reserved_bits(
		static_cast<libgs::websocket::reserved_bit>(data[0] & 0x07U));
	header.op = static_cast<libgs::websocket::opcode>(data[0] & 0x0FU);
	header.payload_size = size > 1 ? data[1] : size;
	if((data[0] & 2U) != 0)
		header.mask = key;
	libgs::websocket::frame_codec_config config;
	config.local_role = (data[0] & 4U) != 0 ?
		libgs::websocket::role::server : libgs::websocket::role::client;
	config.max_frame_size = size > 2 ? data[2] : size;
	config.allowed_rsv = libgs::websocket::reserved_bits(
		static_cast<libgs::websocket::reserved_bit>((data[0] >> 3U) & 0x07U));
	libgs::ignore_unused(libgs::websocket::encode_frame_header(header, config));

	const uint16_t close_code = size > 1 ?
		static_cast<uint16_t>((uint16_t(data[0]) << 8U) | data[1]) : data[0];
	libgs::ignore_unused(libgs::websocket::is_known_opcode(header.op));
	libgs::ignore_unused(libgs::websocket::is_control_opcode(header.op));
	libgs::ignore_unused(libgs::websocket::is_data_opcode(header.op));
	libgs::ignore_unused(libgs::websocket::is_valid_close_code(close_code));
	const auto close_payload = libgs::websocket::encode_close_payload(
		libgs::websocket::close_payload_view {close_code, input});
	if(close_payload)
	{
		const auto decoded = libgs::websocket::decode_close_payload(
			close_payload->buffer());
		if(not decoded or decoded->code != close_code or decoded->reason != input)
			std::abort();
	}

	std::vector<std::byte> destination(size);
	const uint64_t mask_offset = size > 2 ? data[2] : data[0];
	const auto copied = libgs::websocket::mask_copy(
		libgs::mutable_buffer(destination.data(), destination.size()),
		libgs::const_buffer(bytes.data(), bytes.size()), key, mask_offset);
	if(not copied or *copied != bytes.size())
		std::abort();
	if(size != 0)
	{
		std::vector<std::byte> too_small(size - 1, std::byte {0xa5});
		const auto before = too_small;
		const auto rejected = libgs::websocket::mask_copy(
			libgs::mutable_buffer(too_small.data(), too_small.size()),
			libgs::const_buffer(bytes.data(), bytes.size()), key, mask_offset);
		if(rejected or too_small != before)
			std::abort();
	}
	libgs::websocket::apply_mask(
		libgs::mutable_buffer(destination.data(), destination.size()), key, mask_offset);
	if(destination != bytes)
		std::abort();
	libgs::websocket::apply_mask(
		libgs::mutable_buffer(destination.data(), destination.size()), key, size);

	libgs::websocket::permessage_deflate_options options;
	options.server_no_context_takeover = (data[0] & 1U) != 0;
	options.client_no_context_takeover = (data[0] & 2U) != 0;
	options.offer_client_max_window_bits = (data[0] & 4U) != 0;
	options.server_max_window_bits = static_cast<uint8_t>(data[0]);
	options.client_max_window_bits = static_cast<uint8_t>(data[size - 1]);
	const auto extension = libgs::websocket::permessage_deflate_extension(options);
	libgs::ignore_unused(libgs::websocket::is_permessage_deflate_extension(extension));
	return 0;
}
