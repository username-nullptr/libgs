// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/websocket/client.h>
#include <libgs/websocket/server.h>

#include <format>
#include <future>

namespace
{

namespace ws = libgs::websocket;

void owned_handler_round_trip()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	ws::server service(std::move(acceptor));

	ws::upgrade_options options;
	options.supported_subprotocols = {"owned.chat"};
	options.require_subprotocol = true;
	service.on_connection("/echo",
		[](ws::accept_result accepted) -> libgs::awaitable<void>
		{
			auto request = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			co_await accepted.stream.write_text(
				"owned: " + request.body, libgs::use_awaitable);
			libgs::ignore_unused(co_await accepted.stream.close(
				libgs::use_awaitable));
			co_return;
		}, options);
	service.bind({libgs::ip_type::v4, 0}).start();

	const auto port = service.http_server().acceptor_wrap()
		.acceptor().local_endpoint().port();
	ws::client client(context.get_executor());
	auto completed = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			ws::connect_request request(std::format(
				"ws://127.0.0.1:{}/echo", port));
			request.subprotocols = {"owned.chat"};
			auto stream = co_await client.open(
				std::move(request), libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(client.pending_open_count(), 0U);
			LIBGS_TEST_CHECK_EQ(stream.negotiated_subprotocol(), "owned.chat");

			co_await stream.write_text("hello", libgs::use_awaitable);
			auto response = co_await stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(response.body, "owned: hello");
			const auto closed = co_await stream.close(libgs::use_awaitable);
			LIBGS_TEST_CHECK(closed.clean);
			service.stop();
			co_return;
		}, asio::use_future);
	context.run();
	completed.get();
}

void owned_accept_round_trip()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	ws::server service(std::move(acceptor));
	service.bind({libgs::ip_type::v4, 0}).start();
	const auto port = service.http_server().acceptor_wrap()
		.acceptor().local_endpoint().port();

	auto accepted = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			auto connection = co_await service.accept(libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(connection.request.path, "/accept");
			auto message = co_await connection.stream.read<std::string>(
				libgs::use_awaitable);
			co_await connection.stream.write_text(
				message.body, libgs::use_awaitable);
			libgs::ignore_unused(co_await connection.stream.close(
				libgs::use_awaitable));
			co_return;
		}, asio::use_future);

	ws::client client(context.get_executor());
	auto connected = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			auto stream = co_await client.open(std::format(
				"ws://127.0.0.1:{}/accept", port), libgs::use_awaitable);
			co_await stream.write_text("accept-mode", libgs::use_awaitable);
			auto response = co_await stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(response.body, "accept-mode");
			const auto closed = co_await stream.close(libgs::use_awaitable);
			LIBGS_TEST_CHECK(closed.clean);
			service.stop();
			co_return;
		}, asio::use_future);

	context.run();
	accepted.get();
	connected.get();
}

void delivery_modes_and_cancellation()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	ws::server service(std::move(acceptor));

	bool completed = false;
	service.accept([&](libgs::error_code error, ws::accept_result result)
	{
		LIBGS_TEST_CHECK_EQ(error,
			libgs::error_code(asio::error::operation_aborted));
		LIBGS_TEST_CHECK(not result.stream.is_open());
		completed = true;
	});
	LIBGS_TEST_CHECK_EQ(service.pending_accept_count(), 1U);
	LIBGS_TEST_CHECK_THROWS(
		service.on_default([](ws::accept_result) -> libgs::awaitable<void> {
			co_return;
		}), std::logic_error);
	service.cancel();
	context.run();
	LIBGS_TEST_CHECK(completed);
	LIBGS_TEST_CHECK_EQ(service.pending_accept_count(), 0U);

	libgs::io_context_t other_context;
	asio::ip::tcp::acceptor other_acceptor(other_context);
	ws::server callback_service(std::move(other_acceptor));
	callback_service.on_default(
		[](ws::accept_result) -> libgs::awaitable<void> { co_return; });
	bool rejected = false;
	callback_service.accept(
		[&](libgs::error_code error, ws::accept_result result)
		{
			LIBGS_TEST_CHECK_EQ(error,
				std::make_error_code(std::errc::operation_not_supported));
			LIBGS_TEST_CHECK(not result.stream.is_open());
			rejected = true;
		});
	other_context.run();
	LIBGS_TEST_CHECK(rejected);
	libgs::error_code sync_error;
	auto sync_result = callback_service.accept(sync_error);
	LIBGS_TEST_CHECK_EQ(sync_error,
		std::make_error_code(std::errc::operation_not_supported));
	LIBGS_TEST_CHECK(not sync_result.stream.is_open());
}

void pending_handshake_queue()
{
	using namespace std::chrono_literals;
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	ws::server_config config;
	config.pending_handshake_timeout = 1s;
	ws::server service(std::move(acceptor), config);

	// Fix the server in accept mode, then remove the initial waiter so that the
	// incoming request has to wait in the handshake queue.
	asio::cancellation_signal cancellation;
	bool initial_cancelled = false;
	service.accept(asio::bind_cancellation_slot(cancellation.slot(),
		[&](libgs::error_code error, ws::accept_result)
		{
			LIBGS_TEST_CHECK_EQ(error,
				libgs::error_code(asio::error::operation_aborted));
			initial_cancelled = true;
		}));
	cancellation.emit(asio::cancellation_type::all);
	context.poll();
	LIBGS_TEST_CHECK(initial_cancelled);
	context.restart();

	service.bind({libgs::ip_type::v4, 0}).start();
	const auto port = service.http_server().acceptor_wrap()
		.acceptor().local_endpoint().port();
	ws::client client(context.get_executor());
	auto connected = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			auto stream = co_await client.open(std::format(
				"ws://127.0.0.1:{}/queued", port), libgs::use_awaitable);
			co_await stream.write_text("queued", libgs::use_awaitable);
			auto response = co_await stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(response.body, "queued");
			libgs::ignore_unused(co_await stream.close(libgs::use_awaitable));
			service.stop();
			co_return;
		}, asio::use_future);
	auto accepted = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			asio::steady_timer delay(context.get_executor());
			delay.expires_after(10ms);
			co_await delay.async_wait(libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(service.pending_handshake_count(), 1U);
			auto connection = co_await service.accept(libgs::use_awaitable);
			auto message = co_await connection.stream.read<std::string>(
				libgs::use_awaitable);
			co_await connection.stream.write_text(
				message.body, libgs::use_awaitable);
			libgs::ignore_unused(co_await connection.stream.close(
				libgs::use_awaitable));
			co_return;
		}, asio::use_future);

	context.run();
	connected.get();
	accepted.get();
}

void owned_client_cancellation()
{
	using namespace std::chrono_literals;
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context,
		{asio::ip::address_v4::loopback(), 0});
	asio::ip::tcp::socket peer(context);
	acceptor.async_accept(peer, [](libgs::error_code error) {
		LIBGS_TEST_CHECK(not error);
	});

	ws::client_config config;
	config.handshake_timeout = 1s;
	ws::client client(context.get_executor(), config);
	const auto port = acceptor.local_endpoint().port();
	auto opening = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			auto [error, stream] = co_await client.open(
				std::format("ws://127.0.0.1:{}/stall", port),
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(error,
				libgs::error_code(asio::error::operation_aborted));
			LIBGS_TEST_CHECK(not stream.is_open());
			LIBGS_TEST_CHECK_EQ(client.pending_open_count(), 0U);
			co_return;
		}, asio::use_future);
	auto cancellation = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			asio::steady_timer delay(context.get_executor());
			delay.expires_after(10ms);
			co_await delay.async_wait(libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(client.pending_open_count(), 1U);
			client.cancel();
			co_return;
		}, asio::use_future);

	context.run();
	opening.get();
	cancellation.get();
}

void invalid_owned_config()
{
	libgs::io_context_t context;
	ws::client preflight_client(context.get_executor());
	libgs::error_code error;
	auto stream = preflight_client.open(
		ws::connect_request("ftp://example.test/socket"), error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::protocol_not_supported));
	LIBGS_TEST_CHECK(not stream.is_open());
	LIBGS_TEST_CHECK_EQ(preflight_client.pending_open_count(), 0U);

	ws::client_config client_config;
	client_config.stream.read_buffer_size = 0;
	LIBGS_TEST_CHECK_THROWS(ws::client(client_config), std::system_error);

	asio::ip::tcp::acceptor acceptor(context);
	ws::server_config server_config;
	server_config.default_upgrade.stream.read_buffer_size = 0;
	LIBGS_TEST_CHECK_THROWS(
		ws::server(std::move(acceptor), server_config), std::system_error);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"owned handler round trip", owned_handler_round_trip},
		{"owned accept round trip", owned_accept_round_trip},
		{"delivery modes and cancellation", delivery_modes_and_cancellation},
		{"pending handshake queue", pending_handshake_queue},
		{"owned client cancellation", owned_client_cancellation},
		{"invalid owned config", invalid_owned_config},
	});
}
