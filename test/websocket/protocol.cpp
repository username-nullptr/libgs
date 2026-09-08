// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"
#include <libgs/websocket/error.h>
#include <libgs/websocket/protocol/generator.h>
#include <libgs/websocket/protocol/handshake.h>
#include <libgs/websocket/protocol/parser.h>
#include <libgs/websocket/detail/secure_random.h>
#include <array>
#include <cstring>
#include <limits>
#include <string>

namespace ws = libgs::websocket;

namespace
{

[[nodiscard]] uint8_t octet(const libgs::const_buffer &buffer, size_t index)
{
	return static_cast<const uint8_t*>(buffer.data())[index];
}

void check_error(const libgs::error_code &actual, ws::protocol_errc expected)
{
	LIBGS_TEST_CHECK_EQ(actual, ws::make_error_code(expected));
}

void check_error(const libgs::error_code &actual, ws::errc expected)
{
	LIBGS_TEST_CHECK_EQ(actual, ws::make_error_code(expected));
}

void test_error_categories()
{
	const libgs::error_code stream_error = ws::errc::not_open;
	LIBGS_TEST_CHECK_EQ(stream_error.category(), ws::error_category());
	LIBGS_TEST_CHECK_EQ(std::string_view(stream_error.category().name()),
		"libgs::websocket");
	LIBGS_TEST_CHECK(not stream_error.message().empty());
	LIBGS_TEST_CHECK(stream_error == ws::errc::not_open);
	LIBGS_TEST_CHECK(ws::errc::not_open == stream_error);
	LIBGS_TEST_CHECK(stream_error != ws::errc::closed);

	const libgs::error_code protocol_error = ws::protocol_errc::invalid_utf8;
	LIBGS_TEST_CHECK_EQ(protocol_error.category(), ws::protocol_error_category());
	LIBGS_TEST_CHECK_EQ(std::string_view(protocol_error.category().name()),
		"libgs::websocket::protocol");
	LIBGS_TEST_CHECK(not protocol_error.message().empty());
	LIBGS_TEST_CHECK(protocol_error == ws::protocol_errc::invalid_utf8);
	LIBGS_TEST_CHECK(ws::protocol_errc::invalid_utf8 == protocol_error);
	LIBGS_TEST_CHECK(protocol_error != ws::protocol_errc::missing_mask);

#define X_MACRO(e,v,d) \
	{ \
		const libgs::error_code mapped = ws::errc::e; \
		LIBGS_TEST_CHECK_EQ(mapped.value(), v); \
		LIBGS_TEST_CHECK_EQ(mapped.message(), d); \
	}
	LIBGS_WEBSOCKET_ERRC_TABLE
#undef X_MACRO
}

void test_secure_random_source()
{
	auto empty = ws::detail::secure_random_bytes(libgs::mutable_buffer {});
	LIBGS_TEST_CHECK(empty.has_value());

	auto invalid = ws::detail::secure_random_bytes(
		libgs::mutable_buffer(nullptr, 1)
	);
	LIBGS_TEST_CHECK(not invalid.has_value());
	LIBGS_TEST_CHECK_EQ(invalid.error(),
		std::make_error_code(std::errc::invalid_argument));

	std::array<std::byte,32> bytes {};
	auto filled = ws::detail::secure_random_bytes(
		libgs::mutable_buffer(bytes.data(), bytes.size())
	);
	LIBGS_TEST_CHECK(filled.has_value());
}

void test_opcode_and_close_code_helpers()
{
	LIBGS_TEST_CHECK(ws::is_known_opcode(ws::opcode::continuation));
	LIBGS_TEST_CHECK(ws::is_known_opcode(ws::opcode::pong));
	LIBGS_TEST_CHECK(not ws::is_known_opcode(static_cast<ws::opcode>(0x03)));
	LIBGS_TEST_CHECK(ws::is_control_opcode(ws::opcode::ping));
	LIBGS_TEST_CHECK(not ws::is_control_opcode(ws::opcode::text));
	LIBGS_TEST_CHECK(ws::is_data_opcode(ws::opcode::text));
	LIBGS_TEST_CHECK(not ws::is_data_opcode(ws::opcode::continuation));

	LIBGS_TEST_CHECK(ws::is_valid_close_code(1000));
	LIBGS_TEST_CHECK(ws::is_valid_close_code(1014));
	LIBGS_TEST_CHECK(ws::is_valid_close_code(3000));
	LIBGS_TEST_CHECK(ws::is_valid_close_code(4999));
	LIBGS_TEST_CHECK(not ws::is_valid_close_code(999));
	LIBGS_TEST_CHECK(not ws::is_valid_close_code(1005));
	LIBGS_TEST_CHECK(not ws::is_valid_close_code(1015));
	LIBGS_TEST_CHECK(not ws::is_valid_close_code(2000));
	LIBGS_TEST_CHECK(not ws::is_valid_close_code(5000));

	LIBGS_TEST_CHECK_EQ(ws::close_code_for(ws::protocol_errc::invalid_utf8),
		ws::close_code::invalid_payload);
	LIBGS_TEST_CHECK_EQ(ws::close_code_for(ws::protocol_errc::frame_too_large),
		ws::close_code::message_too_big);
	LIBGS_TEST_CHECK_EQ(ws::close_code_for(ws::protocol_errc::missing_mask),
		ws::close_code::protocol_error);
}

void test_frame_header_encoding()
{
	ws::frame_codec_config server_config;
	server_config.local_role = ws::role::server;

	auto encoded = ws::encode_frame_header({
		.fin = true,
		.op = ws::opcode::binary,
		.payload_size = 125,
	}, server_config);
	LIBGS_TEST_CHECK(encoded.has_value());
	LIBGS_TEST_CHECK_EQ(encoded->size, 2);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 0), 0x82);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 1), 125);

	encoded = ws::encode_frame_header({
		.fin = true,
		.op = ws::opcode::binary,
		.payload_size = 126,
	}, server_config);
	LIBGS_TEST_CHECK(encoded.has_value());
	LIBGS_TEST_CHECK_EQ(encoded->size, 4);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 1), 126);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 2), 0);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 3), 126);

	server_config.max_frame_size = 0;
	encoded = ws::encode_frame_header({
		.fin = true,
		.op = ws::opcode::binary,
		.payload_size = 0x10000,
	}, server_config);
	LIBGS_TEST_CHECK(encoded.has_value());
	LIBGS_TEST_CHECK_EQ(encoded->size, 10);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 1), 127);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 7), 1);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 8), 0);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 9), 0);

	const ws::masking_key key {{
		std::byte {0x12}, std::byte {0x34}, std::byte {0x56}, std::byte {0x78}
	}};
	encoded = ws::encode_frame_header({
		.fin = true,
		.rsv = ws::reserved_bit::rsv1,
		.op = ws::opcode::text,
		.payload_size = 5,
		.mask = key,
	}, ws::frame_codec_config {
		.local_role = ws::role::client,
		.max_frame_size = 16 * 1024 * 1024,
		.allowed_rsv = ws::reserved_bit::rsv1,
	});
	LIBGS_TEST_CHECK(encoded.has_value());
	LIBGS_TEST_CHECK_EQ(encoded->size, 6);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 0), 0xC1);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 1), 0x85);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 2), 0x12);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 5), 0x78);
}

void test_frame_header_errors()
{
	ws::frame_codec_config server_config;
	server_config.local_role = ws::role::server;

	auto result = ws::encode_frame_header({
		.op = static_cast<ws::opcode>(0x03),
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::reserved_opcode);

	result = ws::encode_frame_header({
		.rsv = ws::reserved_bit::rsv1,
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::unexpected_rsv);

	result = ws::encode_frame_header({
		.fin = false,
		.op = ws::opcode::ping,
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::fragmented_control_frame);

	result = ws::encode_frame_header({
		.op = ws::opcode::close,
		.payload_size = 126,
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::control_payload_too_large);

	result = ws::encode_frame_header({
		.payload_size = server_config.max_frame_size + 1,
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::frame_too_large);

	result = ws::encode_frame_header({}, ws::frame_codec_config {
		.local_role = ws::role::client,
	});
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::missing_mask);

	result = ws::encode_frame_header({
		.mask = ws::masking_key {},
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::unexpected_mask);

	server_config.max_frame_size = 0;
	result = ws::encode_frame_header({
		.payload_size = std::numeric_limits<uint64_t>::max(),
	}, server_config);
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::invalid_64bit_length);
}

void test_masking()
{
	const ws::masking_key key {{
		std::byte {0x37}, std::byte {0xFA}, std::byte {0x21}, std::byte {0x3D}
	}};
	std::array<std::byte,5> payload {
		std::byte {'H'}, std::byte {'e'}, std::byte {'l'}, std::byte {'l'}, std::byte {'o'}
	};
	ws::apply_mask(libgs::mutable_buffer(payload.data(), payload.size()), key);
	LIBGS_TEST_CHECK_EQ(std::to_integer<uint8_t>(payload[0]), 0x7F);
	LIBGS_TEST_CHECK_EQ(std::to_integer<uint8_t>(payload[1]), 0x9F);
	LIBGS_TEST_CHECK_EQ(std::to_integer<uint8_t>(payload[2]), 0x4D);
	LIBGS_TEST_CHECK_EQ(std::to_integer<uint8_t>(payload[3]), 0x51);
	LIBGS_TEST_CHECK_EQ(std::to_integer<uint8_t>(payload[4]), 0x58);
	ws::apply_mask(libgs::mutable_buffer(payload.data(), payload.size()), key);
	LIBGS_TEST_CHECK(std::memcmp(payload.data(), "Hello", payload.size()) == 0);

	std::array<std::byte,3> source {
		std::byte {0x10}, std::byte {0x20}, std::byte {0x30}
	};
	std::array<std::byte,3> destination {};
	auto copied = ws::mask_copy (
		libgs::mutable_buffer(destination.data(), destination.size()),
		libgs::const_buffer(source.data(), source.size()), key, 3
	);
	LIBGS_TEST_CHECK(copied.has_value());
	LIBGS_TEST_CHECK_EQ(*copied, source.size());
	LIBGS_TEST_CHECK_EQ(destination[0], source[0] ^ key.bytes[3]);
	LIBGS_TEST_CHECK_EQ(destination[1], source[1] ^ key.bytes[0]);
	LIBGS_TEST_CHECK_EQ(destination[2], source[2] ^ key.bytes[1]);

	std::array<std::byte,2> too_small {std::byte {0xAA}, std::byte {0xBB}};
	copied = ws::mask_copy (
		libgs::mutable_buffer(too_small.data(), too_small.size()),
		libgs::const_buffer(source.data(), source.size()), key
	);
	LIBGS_TEST_CHECK(not copied.has_value());
	LIBGS_TEST_CHECK_EQ(copied.error(),
		std::make_error_code(std::errc::no_buffer_space));
	LIBGS_TEST_CHECK_EQ(too_small[0], std::byte {0xAA});
	LIBGS_TEST_CHECK_EQ(too_small[1], std::byte {0xBB});
}

void test_close_payload()
{
	auto encoded = ws::encode_close_payload(ws::close_frame {
		ws::close_code::normal_closure, "done"
	});
	LIBGS_TEST_CHECK(encoded.has_value());
	LIBGS_TEST_CHECK_EQ(encoded->size, 6);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 0), 0x03);
	LIBGS_TEST_CHECK_EQ(octet(encoded->buffer(), 1), 0xE8);

	auto decoded = ws::decode_close_payload(encoded->buffer());
	LIBGS_TEST_CHECK(decoded.has_value());
	LIBGS_TEST_CHECK(decoded->code.has_value());
	LIBGS_TEST_CHECK_EQ(*decoded->code, 1000);
	LIBGS_TEST_CHECK_EQ(decoded->reason, "done");

	decoded = ws::decode_close_payload(libgs::const_buffer {});
	LIBGS_TEST_CHECK(decoded.has_value());
	LIBGS_TEST_CHECK(not decoded->code.has_value());
	LIBGS_TEST_CHECK(decoded->reason.empty());

	const std::array<std::byte,1> one_byte {std::byte {0x03}};
	decoded = ws::decode_close_payload({one_byte.data(), one_byte.size()});
	LIBGS_TEST_CHECK(not decoded.has_value());
	check_error(decoded.error(), ws::protocol_errc::invalid_close_payload);

	const std::array<std::byte,2> forbidden_code {
		std::byte {0x03}, std::byte {0xED}
	};
	decoded = ws::decode_close_payload({forbidden_code.data(), forbidden_code.size()});
	LIBGS_TEST_CHECK(not decoded.has_value());
	check_error(decoded.error(), ws::protocol_errc::invalid_close_payload);

	encoded = ws::encode_close_payload(ws::close_payload_view {
		.code = std::nullopt,
		.reason = "reason without code",
	});
	LIBGS_TEST_CHECK(not encoded.has_value());
	check_error(encoded.error(), ws::protocol_errc::invalid_close_payload);

	const std::string overlong_utf8("\xC0\x80", 2);
	encoded = ws::encode_close_payload(ws::close_payload_view {
		.code = 1000,
		.reason = overlong_utf8,
	});
	LIBGS_TEST_CHECK(not encoded.has_value());
	check_error(encoded.error(), ws::protocol_errc::invalid_utf8);

	const std::string too_long(124, 'a');
	encoded = ws::encode_close_payload(ws::close_payload_view {
		.code = 1000,
		.reason = too_long,
	});
	LIBGS_TEST_CHECK(not encoded.has_value());
	check_error(encoded.error(), ws::protocol_errc::control_payload_too_large);

	const std::array<std::byte,5> surrogate_utf8 {
		std::byte {0x03}, std::byte {0xE8},
		std::byte {0xED}, std::byte {0xA0}, std::byte {0x80}
	};
	decoded = ws::decode_close_payload({surrogate_utf8.data(), surrogate_utf8.size()});
	LIBGS_TEST_CHECK(not decoded.has_value());
	check_error(decoded.error(), ws::protocol_errc::invalid_utf8);
}

void test_opening_handshake_round_trip()
{
	const std::string nonce_text = "the sample nonce";
	std::array<std::byte,16> nonce {};
	std::memcpy(nonce.data(), nonce_text.data(), nonce.size());
	auto client_key = ws::make_client_key(nonce);
	LIBGS_TEST_CHECK(client_key.has_value());
	LIBGS_TEST_CHECK_EQ(*client_key, "dGhlIHNhbXBsZSBub25jZQ==");

	auto accept_key = ws::make_accept_key(*client_key);
	LIBGS_TEST_CHECK(accept_key.has_value());
	LIBGS_TEST_CHECK_EQ(*accept_key, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");

	ws::extension compression;
	compression.name = "permessage-deflate";
	compression.parameters = {
		{.name = "client_max_window_bits"},
		{.name = "mode", .value = "fast"},
	};
	ws::opening_request request {
		.key = *client_key,
		.subprotocols = {"chat", "superchat"},
		.extensions = {compression},
	};

	auto request_headers = ws::make_opening_request_headers(request);
	LIBGS_TEST_CHECK(request_headers.has_value());
	LIBGS_TEST_CHECK_EQ(request_headers->at("Connection").to_string(), "Upgrade");
	LIBGS_TEST_CHECK_EQ(request_headers->at("Upgrade").to_string(), "websocket");
	LIBGS_TEST_CHECK_EQ(request_headers->at("Sec-WebSocket-Version").to_string(), "13");
	LIBGS_TEST_CHECK_EQ(request_headers->at("Sec-WebSocket-Protocol").to_string(),
		"chat, superchat");
	LIBGS_TEST_CHECK_EQ(request_headers->at("Sec-WebSocket-Extensions").to_string(),
		"permessage-deflate; client_max_window_bits; mode=fast");

	(*request_headers)["Connection"] = "keep-alive, uPgRaDe";
	(*request_headers)["Upgrade"] = "WebSocket";
	(*request_headers)["Sec-WebSocket-Extensions"] =
		"permessage-deflate; client_max_window_bits; mode=\"fa\\st\"";
	auto parsed_request = ws::parse_opening_request(libgs::http::method::get,
		libgs::http::version::v11, *request_headers);
	LIBGS_TEST_CHECK(parsed_request.has_value());
	LIBGS_TEST_CHECK_EQ(parsed_request->key, *client_key);
	LIBGS_TEST_CHECK_EQ(parsed_request->subprotocols.size(), size_t {2});
	LIBGS_TEST_CHECK_EQ(parsed_request->subprotocols[1], "superchat");
	LIBGS_TEST_CHECK_EQ(parsed_request->extensions.size(), size_t {1});
	LIBGS_TEST_CHECK_EQ(parsed_request->extensions[0].parameters.size(), size_t {2});
	LIBGS_TEST_CHECK(not parsed_request->extensions[0].parameters[0].value);
	LIBGS_TEST_CHECK_EQ(*parsed_request->extensions[0].parameters[1].value, "fast");

	ws::opening_response response {
		.subprotocol = "chat",
		.extensions = {compression},
	};
	auto response_headers = ws::make_opening_response_headers(*parsed_request, response);
	LIBGS_TEST_CHECK(response_headers.has_value());
	LIBGS_TEST_CHECK_EQ(response_headers->at("Sec-WebSocket-Accept").to_string(),
		"s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
	auto parsed_response = ws::parse_opening_response (
		libgs::http::status::switching_protocols,
		*response_headers, *parsed_request
	);
	LIBGS_TEST_CHECK(parsed_response.has_value());
	LIBGS_TEST_CHECK_EQ(parsed_response->subprotocol.value_or(""), "chat");
	LIBGS_TEST_CHECK_EQ(parsed_response->extensions.size(), size_t {1});
}

void test_opening_handshake_errors()
{
	ws::opening_request request {
		.key = "dGhlIHNhbXBsZSBub25jZQ==",
		.subprotocols = {"chat", "superchat"},
	};
	auto generated = ws::make_opening_request_headers(request);
	LIBGS_TEST_CHECK(generated.has_value());

	auto parsed = ws::parse_opening_request(libgs::http::method::post,
		libgs::http::version::v11, *generated);
	LIBGS_TEST_CHECK(not parsed.has_value());
	check_error(parsed.error(), ws::errc::invalid_upgrade);

	auto headers = *generated;
	headers["Sec-WebSocket-Version"] = "12";
	parsed = ws::parse_opening_request(libgs::http::method::get,
		libgs::http::version::v11, headers);
	LIBGS_TEST_CHECK(not parsed.has_value());
	check_error(parsed.error(), ws::errc::unsupported_version);

	headers = *generated;
	headers["Sec-WebSocket-Key"] = "dGhlIHNhbXBsZSBub25jZQ=A";
	parsed = ws::parse_opening_request(libgs::http::method::get,
		libgs::http::version::v11, headers);
	LIBGS_TEST_CHECK(not parsed.has_value());
	check_error(parsed.error(), ws::errc::invalid_upgrade);

	headers = *generated;
	headers["Sec-WebSocket-Protocol"] = "chat, chat";
	parsed = ws::parse_opening_request(libgs::http::method::get,
		libgs::http::version::v11, headers);
	LIBGS_TEST_CHECK(not parsed.has_value());
	check_error(parsed.error(), ws::errc::invalid_upgrade);

	headers = *generated;
	headers["Sec-WebSocket-Extensions"] = "permessage-deflate; mode=\"not valid\"";
	parsed = ws::parse_opening_request(libgs::http::method::get,
		libgs::http::version::v11, headers);
	LIBGS_TEST_CHECK(not parsed.has_value());
	check_error(parsed.error(), ws::errc::invalid_upgrade);

	request.subprotocols = {"chat", "chat"};
	generated = ws::make_opening_request_headers(request);
	LIBGS_TEST_CHECK(not generated.has_value());
	check_error(generated.error(), ws::errc::invalid_upgrade);

	request.subprotocols = {"chat", "superchat"};
	auto response_headers = ws::make_opening_response_headers(request, {});
	LIBGS_TEST_CHECK(response_headers.has_value());
	auto response = ws::parse_opening_response(libgs::http::status::ok,
		*response_headers, request);
	LIBGS_TEST_CHECK(not response.has_value());
	check_error(response.error(), ws::errc::handshake_rejected);

	headers = *response_headers;
	headers["Sec-WebSocket-Accept"] = "incorrect";
	response = ws::parse_opening_response(libgs::http::status::switching_protocols,
		headers, request);
	LIBGS_TEST_CHECK(not response.has_value());
	check_error(response.error(), ws::errc::invalid_accept_key);

	headers = *response_headers;
	headers["Sec-WebSocket-Protocol"] = "other";
	response = ws::parse_opening_response(libgs::http::status::switching_protocols,
		headers, request);
	LIBGS_TEST_CHECK(not response.has_value());
	check_error(response.error(), ws::errc::unsupported_subprotocol);

	headers = *response_headers;
	headers["Sec-WebSocket-Protocol"] = "chat, superchat";
	response = ws::parse_opening_response(libgs::http::status::switching_protocols,
		headers, request);
	LIBGS_TEST_CHECK(not response.has_value());
	check_error(response.error(), ws::errc::invalid_upgrade);
}

void test_incremental_frame_parser()
{
	ws::frame_parser parser(ws::frame_codec_config {
		.local_role = ws::role::server,
	});
	std::array<std::byte,1> chunk1 {std::byte {0x81}};
	std::array<std::byte,3> chunk2 {
		std::byte {0x85}, std::byte {0x37}, std::byte {0xFA}
	};
	std::array<std::byte,4> chunk3 {
		std::byte {0x21}, std::byte {0x3D}, std::byte {0x7F}, std::byte {0x9F}
	};
	std::array<std::byte,5> chunk4 {
		std::byte {0x4D}, std::byte {0x51}, std::byte {0x58},
		std::byte {0xAA}, std::byte {0xBB}
	};

	auto result = parser.parse({chunk1.data(), chunk1.size()});
	LIBGS_TEST_CHECK(result.has_value());
	LIBGS_TEST_CHECK_EQ(result->consumed, size_t {1});
	LIBGS_TEST_CHECK(not result->header_ready);
	LIBGS_TEST_CHECK(not result->frame_finished);

	result = parser.parse({chunk2.data(), chunk2.size()});
	LIBGS_TEST_CHECK(result.has_value());
	LIBGS_TEST_CHECK_EQ(result->consumed, chunk2.size());
	LIBGS_TEST_CHECK(not result->header_ready);

	result = parser.parse({chunk3.data(), chunk3.size()});
	LIBGS_TEST_CHECK(result.has_value());
	LIBGS_TEST_CHECK(result->header_ready);
	LIBGS_TEST_CHECK(not result->frame_finished);
	LIBGS_TEST_CHECK_EQ(result->consumed, chunk3.size());
	LIBGS_TEST_CHECK_EQ(result->payload.size(), size_t {2});
	LIBGS_TEST_CHECK_EQ(result->payload_offset, uint64_t {0});
	LIBGS_TEST_CHECK_EQ(parser.payload_remaining(), uint64_t {3});
	LIBGS_TEST_CHECK_EQ(parser.header().op, ws::opcode::text);
	LIBGS_TEST_CHECK_EQ(parser.header().payload_size, uint64_t {5});
	LIBGS_TEST_CHECK(parser.header().mask.has_value());
	ws::apply_mask(result->payload, *parser.header().mask, result->payload_offset);
	LIBGS_TEST_CHECK(std::memcmp(chunk3.data() + 2, "He", 2) == 0);

	result = parser.parse({chunk4.data(), chunk4.size()});
	LIBGS_TEST_CHECK(result.has_value());
	LIBGS_TEST_CHECK(not result->header_ready);
	LIBGS_TEST_CHECK(result->frame_finished);
	LIBGS_TEST_CHECK_EQ(result->consumed, size_t {3});
	LIBGS_TEST_CHECK_EQ(result->payload.size(), size_t {3});
	LIBGS_TEST_CHECK_EQ(result->payload_offset, uint64_t {2});
	LIBGS_TEST_CHECK_EQ(parser.payload_remaining(), uint64_t {0});
	ws::apply_mask(result->payload, *parser.header().mask, result->payload_offset);
	LIBGS_TEST_CHECK(std::memcmp(chunk4.data(), "llo", 3) == 0);
}

template <size_t Size>
void check_parser_error(std::array<std::byte,Size> wire,
	ws::frame_codec_config config, ws::protocol_errc expected)
{
	ws::frame_parser parser(config);
	auto result = parser.parse({wire.data(), wire.size()});
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), expected);
	LIBGS_TEST_CHECK(parser.failed());
	check_error(parser.last_error(), expected);
}

void test_frame_parser_errors()
{
	check_parser_error(std::array {
		std::byte {0x83}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::reserved_opcode);

	check_parser_error(std::array {
		std::byte {0xC2}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::unexpected_rsv);

	check_parser_error(std::array {
		std::byte {0x82}, std::byte {0x80},
		std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::unexpected_mask);

	check_parser_error(std::array {
		std::byte {0x82}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::server},
		ws::protocol_errc::missing_mask);

	check_parser_error(std::array {
		std::byte {0x82}, std::byte {0x7E}, std::byte {0x00}, std::byte {0x7D}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::noncanonical_length);

	check_parser_error(std::array {
		std::byte {0x82}, std::byte {0x7F}, std::byte {0x80}, std::byte {0x00},
		std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00},
		std::byte {0x00}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::invalid_64bit_length);

	check_parser_error(std::array {
		std::byte {0x09}, std::byte {0x00}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::fragmented_control_frame);

	check_parser_error(std::array {
		std::byte {0x88}, std::byte {0x7E}, std::byte {0x00}, std::byte {0x7E}
	}, ws::frame_codec_config {.local_role = ws::role::client},
		ws::protocol_errc::control_payload_too_large);

	check_parser_error(std::array {
		std::byte {0x82}, std::byte {0x7E}, std::byte {0x04}, std::byte {0x00}
	}, ws::frame_codec_config {
		.local_role = ws::role::client,
		.max_frame_size = 1000,
	}, ws::protocol_errc::frame_too_large);
}

void parse_empty_frame(ws::frame_parser &parser, uint8_t first)
{
	std::array<std::byte,2> wire {
		static_cast<std::byte>(first), std::byte {0x00}
	};
	auto result = parser.parse({wire.data(), wire.size()});
	LIBGS_TEST_CHECK(result.has_value());
	LIBGS_TEST_CHECK(result->header_ready);
	LIBGS_TEST_CHECK(result->frame_finished);
	LIBGS_TEST_CHECK_EQ(result->consumed, wire.size());
}

void test_frame_fragmentation_state()
{
	ws::frame_parser parser(ws::frame_codec_config {
		.local_role = ws::role::client,
	});
	parse_empty_frame(parser, 0x01);
	parse_empty_frame(parser, 0x89);

	std::array<std::byte,2> data_during_fragmentation {
		std::byte {0x82}, std::byte {0x00}
	};
	auto result = parser.parse({data_during_fragmentation.data(),
		data_during_fragmentation.size()});
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::data_during_fragmentation);

	std::array<std::byte,2> continuation {
		std::byte {0x80}, std::byte {0x00}
	};
	result = parser.parse({continuation.data(), continuation.size()});
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::data_during_fragmentation);

	parser.reset();
	result = parser.parse({continuation.data(), continuation.size()});
	LIBGS_TEST_CHECK(not result.has_value());
	check_error(result.error(), ws::protocol_errc::unexpected_continuation);

	parser.reset();
	parse_empty_frame(parser, 0x01);
	parse_empty_frame(parser, 0x00);
	parse_empty_frame(parser, 0x80);
	parse_empty_frame(parser, 0x82);
	LIBGS_TEST_CHECK(not parser.failed());
}

} //namespace

int main()
{
	return libgs::test::run({
		{"error categories", test_error_categories},
		{"secure random source", test_secure_random_source},
		{"opcode and close code helpers", test_opcode_and_close_code_helpers},
		{"frame header encoding", test_frame_header_encoding},
		{"frame header errors", test_frame_header_errors},
		{"masking", test_masking},
		{"Close payload", test_close_payload},
		{"opening handshake round trip", test_opening_handshake_round_trip},
		{"opening handshake errors", test_opening_handshake_errors},
		{"incremental frame parser", test_incremental_frame_parser},
		{"frame parser errors", test_frame_parser_errors},
		{"frame fragmentation state", test_frame_fragmentation_state},
	});
}
