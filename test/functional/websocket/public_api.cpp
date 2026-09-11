// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs.h>
#include <libgs/websocket/protocol/generator.h>
#include <libgs/websocket/protocol/handshake.h>
#include <libgs/websocket/protocol/parser.h>

namespace
{

namespace ws = libgs::websocket;

static_assert(std::same_as<ws::stream::executor_t, asio::any_io_executor>);
static_assert(std::movable<ws::stream>);
static_assert(not std::copy_constructible<ws::stream>);
static_assert(std::movable<ws::client>);
static_assert(not std::copy_constructible<ws::client>);
static_assert(std::is_error_code_enum_v<ws::errc>);
static_assert(std::is_error_code_enum_v<ws::protocol_errc>);

void umbrella_and_value_types()
{
	ws::stream_config stream_config;
	LIBGS_TEST_CHECK(stream_config.max_frame_size > 0);
	LIBGS_TEST_CHECK(stream_config.max_message_size > 0);
	LIBGS_TEST_CHECK(stream_config.read_buffer_size > 0);

	ws::connect_request request("https://example.test/socket?mode=public");
	request.subprotocols = {"public.v1"};
	request.max_redirects = 2;
	LIBGS_TEST_CHECK_EQ(request.endpoint.protocol(), "https");
	LIBGS_TEST_CHECK_EQ(request.subprotocols.front(), "public.v1");

	ws::upgrade_options upgrade;
	upgrade.supported_subprotocols = request.subprotocols;
	upgrade.require_subprotocol = true;
	LIBGS_TEST_CHECK(upgrade.require_subprotocol);

	auto compression = ws::permessage_deflate_extension();
	LIBGS_TEST_CHECK(ws::is_permessage_deflate_extension(compression));
	std::swap(compression.parameters[0], compression.parameters[1]);
	LIBGS_TEST_CHECK(ws::is_permessage_deflate_extension(compression));
	compression.parameters.front().value = "1";
	LIBGS_TEST_CHECK(not ws::is_permessage_deflate_extension(compression));
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
	static_assert(ws::permessage_deflate_available_v);
#else
	static_assert(not ws::permessage_deflate_available_v);
#endif

	ws::close_frame close(ws::close_code::normal_closure, "done");
	auto encoded = ws::encode_close_payload(close);
	LIBGS_TEST_CHECK(encoded.has_value());
	auto decoded = ws::decode_close_payload(encoded->buffer());
	LIBGS_TEST_CHECK(decoded.has_value());
	LIBGS_TEST_CHECK_EQ(decoded->code.value_or(0), 1000);
	LIBGS_TEST_CHECK_EQ(decoded->reason, "done");
}

void executor_bound_public_objects()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::idle);
	LIBGS_TEST_CHECK(stream.get_executor() == context.get_executor());

	ws::client client(context.get_executor());
	LIBGS_TEST_CHECK(client.get_executor() == context.get_executor());
	LIBGS_TEST_CHECK_EQ(client.pending_open_count(), 0U);

	asio::ip::tcp::acceptor acceptor(context);
	ws::server server(std::move(acceptor));
	LIBGS_TEST_CHECK(server.get_executor() == context.get_executor());
	LIBGS_TEST_CHECK_EQ(server.pending_accept_count(), 0U);
	LIBGS_TEST_CHECK_EQ(server.pending_handshake_count(), 0U);
}

void asynchronous_completion_signatures()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());

	bool read_completed = false;
	stream.read<std::string>([&](libgs::error_code error,
		ws::basic_message<std::string> message)
	{
		LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::not_open));
		LIBGS_TEST_CHECK(message.body.empty());
		read_completed = true;
	});

	bool write_completed = false;
	stream.write_text("public", [&](libgs::error_code error, size_t written)
	{
		LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::not_open));
		LIBGS_TEST_CHECK_EQ(written, 0U);
		write_completed = true;
	});

	bool frame_completed = false;
	stream.read_frame<std::string>([&](libgs::error_code error,
		ws::basic_data_frame<std::string> frame)
	{
		LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::not_open));
		LIBGS_TEST_CHECK(frame.body.empty());
		frame_completed = true;
	});

	bool close_completed = false;
	stream.close([&](libgs::error_code error, ws::close_info info)
	{
		LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::not_open));
		LIBGS_TEST_CHECK(not info.clean);
		close_completed = true;
	});

	LIBGS_TEST_CHECK(not read_completed);
	LIBGS_TEST_CHECK(not write_completed);
	LIBGS_TEST_CHECK(not frame_completed);
	LIBGS_TEST_CHECK(not close_completed);
	context.run();
	LIBGS_TEST_CHECK(read_completed);
	LIBGS_TEST_CHECK(write_completed);
	LIBGS_TEST_CHECK(frame_completed);
	LIBGS_TEST_CHECK(close_completed);
}

void non_default_constructible_executor_errors()
{
	libgs::io_context_t context;
	using executor_t = asio::strand<libgs::io_context_t::executor_type>;
	auto executor = asio::make_strand(context);
	ws::basic_client<executor_t> client(executor);

	libgs::error_code error;
	auto stream = client.open(ws::connect_request(
		"ftp://example.test/socket"), error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::protocol_not_supported));
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::idle);
	LIBGS_TEST_CHECK(stream.get_executor() == executor);

	bool completed = false;
	ws::connect_request expired("ws://example.test/socket");
	expired.handshake_timeout = std::chrono::milliseconds::zero();
	client.open(std::move(expired),
		[&](libgs::error_code callback_error,
			ws::basic_stream<executor_t> callback_stream)
		{
			LIBGS_TEST_CHECK_EQ(callback_error,
				libgs::error_code(asio::error::timed_out));
			LIBGS_TEST_CHECK_EQ(callback_stream.state(),
				ws::connection_state::idle);
			LIBGS_TEST_CHECK(callback_stream.get_executor() == executor);
			completed = true;
		});
	LIBGS_TEST_CHECK(not completed);
	context.run();
	LIBGS_TEST_CHECK(completed);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"umbrella and value types", umbrella_and_value_types},
		{"executor-bound public objects", executor_bound_public_objects},
		{"asynchronous completion signatures", asynchronous_completion_signatures},
		{"non-default-constructible executor errors",
			non_default_constructible_executor_errors},
	});
}
