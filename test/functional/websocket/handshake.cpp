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

void mixed_http_upgrade_round_trip()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	libgs::http::server service(std::move(acceptor));
	auto service_config = service.config();
	service_config.keepalive_time = std::chrono::milliseconds(20);
	service.set_config(service_config);
	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/echo",
		[](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.supported_subprotocols = {"chat"};
			options.response_headers["X-WebSocket-Test"] = "accepted";
			options.origin_validator = [](std::optional<std::string_view> origin)
				-> ws::upgrade_validation_result
			{
				if(origin == "https://example.test")
					return std::nullopt;
				return ws::upgrade_rejection {
					.status = libgs::http::status::forbidden,
					.body = "origin rejected"
				};
			};
			auto [upgrade_error, accepted] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			if(upgrade_error)
				co_return;
			auto message = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			if(message.type != ws::message_type::text or message.body != "hello")
				throw std::runtime_error(std::format(
					"unexpected WebSocket request payload: type={}, body={}",
					static_cast<unsigned>(message.type), message.body));
			co_await accepted.stream.write_text("world", libgs::use_awaitable);
			co_return;
		})
		.on_request<libgs::http::method::get>("/redirect",
		[](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			http_context.response()
				.set_status(libgs::http::status::found)
				.set_header(libgs::http::header::location, "/echo");
			co_await http_context.response().write(libgs::use_awaitable);
			co_return;
		})
		.on_request<libgs::http::method::get>("/reject",
		[](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.request_validator = [](const ws::request_info&)
				-> ws::upgrade_validation_result
			{
				return ws::upgrade_rejection {
					.status = libgs::http::status::forbidden,
					.headers = {{"X-WebSocket-Test", "rejected"}},
					.body = "denied"
				};
			};
			auto [error, rejected] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(rejected);
			if(error != ws::errc::handshake_rejected)
				throw std::runtime_error("unexpected server rejection result");
			co_return;
		})
		.on_server_error([](libgs::error_code error) {
			std::cerr << "server error: " << error.message() << '\n';
			return true;
		})
		.on_service_error([](libgs::http::server::context_t&,
			const std::exception &error) {
			std::cerr << "service error: " << error.what() << '\n';
			return true;
		})
		.start();

	const auto port = service.acceptor_wrap().acceptor().local_endpoint().port();
	const auto base = std::format("ws://127.0.0.1:{}", port);
	const auto http_base = std::format("http://127.0.0.1:{}", port);
	libgs::http::client http_client(context.get_executor());
	auto completed = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		try
		{
			ws::connect_request request(http_base + "/echo?value=42");
			request.subprotocols = {"superchat", "chat"};
			request.request_options.set_header(
				libgs::http::header::origin, "https://example.test");
			ws::open_diagnostics diagnostics;
			auto stream = co_await ws::open(http_client, std::move(request),
				diagnostics, libgs::use_awaitable);
			LIBGS_TEST_CHECK(stream.is_open());
			LIBGS_TEST_CHECK_EQ(stream.negotiated_subprotocol(), "chat");
			LIBGS_TEST_CHECK_EQ(diagnostics.endpoint.protocol(), "ws");
			LIBGS_TEST_CHECK(diagnostics.reply);
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
				libgs::http::status::switching_protocols);
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->header("X-WebSocket-Test")
				->to_string(), "accepted");
			LIBGS_TEST_CHECK(not diagnostics.reply->lease().is_valid());

			co_await stream.write_text("hello", libgs::use_awaitable);
			auto response = co_await stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(response.type, ws::message_type::text);
			LIBGS_TEST_CHECK_EQ(response.body, "world");
			stream.shutdown();

			ws::open_diagnostics rejected_diagnostics;
			auto [rejection_error, rejected_stream] = co_await ws::open(
				http_client, ws::connect_request(base + "/reject"),
				rejected_diagnostics, asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(rejection_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not rejected_stream.is_open());
			LIBGS_TEST_CHECK(rejected_diagnostics.reply);
			LIBGS_TEST_CHECK_EQ(rejected_diagnostics.reply->status(),
				libgs::http::status::forbidden);
			LIBGS_TEST_CHECK_EQ(co_await rejected_diagnostics.reply
				->read<std::string>(libgs::use_awaitable), "denied");

			ws::open_diagnostics redirect_diagnostics;
			auto [redirect_error, redirect_idle] = co_await ws::open(
				http_client, ws::connect_request(base + "/redirect"),
				redirect_diagnostics, asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(redirect_error,
				ws::make_error_code(ws::errc::redirect_limit_exceeded));
			LIBGS_TEST_CHECK(not redirect_idle.is_open());
			LIBGS_TEST_CHECK_EQ(redirect_diagnostics.reply->status(),
				libgs::http::status::found);

			ws::connect_request followed(base + "/redirect");
			followed.max_redirects = 1;
			followed.subprotocols = {"chat"};
			followed.request_options.set_header(
				libgs::http::header::origin, "https://example.test");
			auto redirected_stream = co_await ws::open(
				http_client, std::move(followed), redirect_diagnostics,
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(redirect_diagnostics.endpoint.path(), "/echo");
			co_await redirected_stream.write_text("hello", libgs::use_awaitable);
			auto redirected_response = co_await redirected_stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(redirected_response.body, "world");
			redirected_stream.shutdown();

			auto malformed = co_await http_client.request_get(
				std::format("http://127.0.0.1:{}/echo", port),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(co_await malformed->wait_reply(
				libgs::use_awaitable), libgs::http::status::bad_request);

			libgs::http::request_arg old_version_options;
			old_version_options
				.set_header(libgs::http::header::connection, "Upgrade")
				.set_header(libgs::http::header::upgrade, "websocket")
				.set_header("Sec-WebSocket-Version", "12")
				.set_header("Sec-WebSocket-Key",
					"dGhlIHNhbXBsZSBub25jZQ==");
			auto old_version = co_await http_client.request_get(
				{std::format("http://127.0.0.1:{}/echo", port),
					std::move(old_version_options)}, libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(co_await old_version->wait_reply(
				libgs::use_awaitable), libgs::http::status::upgrade_required);
			LIBGS_TEST_CHECK_EQ(old_version->reply()
				->header("Sec-WebSocket-Version")->to_string(), "13");

			ws::connect_request expired(base + "/echo");
			expired.handshake_timeout = std::chrono::milliseconds::zero();
			auto [timeout_error, idle_stream] = co_await ws::open(
				http_client, std::move(expired),
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(timeout_error,
				libgs::error_code(asio::error::timed_out));
			LIBGS_TEST_CHECK(not idle_stream.is_open());

			ws::connect_request conflicting(base + "/echo");
			conflicting.request_options.set_header("Sec-WebSocket-Key", "owned");
			auto [header_error, another_idle_stream] = co_await ws::open(
				http_client, std::move(conflicting),
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(header_error,
				ws::make_error_code(ws::errc::invalid_upgrade));
			LIBGS_TEST_CHECK(not another_idle_stream.is_open());
		}
		catch(...)
		{
			service.stop();
			throw;
		}
		service.stop();
		co_return;
	}, asio::use_future);
	context.run();
	completed.get();
}

void client_preflight_sync()
{
	libgs::io_context_t context;
	libgs::http::client http_client(context.get_executor());
	libgs::error_code error;

	ws::connect_request unsupported("ftp://example.test/socket");
	auto stream = ws::open(http_client, std::move(unsupported), error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::protocol_not_supported));
	LIBGS_TEST_CHECK(not stream.is_open());

	ws::open_diagnostics diagnostics;
	ws::connect_request expired("http://example.test/socket");
	expired.handshake_timeout = std::chrono::milliseconds::zero();
	stream = ws::open(http_client, std::move(expired), diagnostics, error);
	LIBGS_TEST_CHECK_EQ(error, libgs::error_code(asio::error::timed_out));
	LIBGS_TEST_CHECK(not stream.is_open());
	LIBGS_TEST_CHECK_EQ(diagnostics.endpoint.protocol(), "ws");

	ws::connect_request conflicting("ws://example.test/socket");
	conflicting.request_options.set_header("Connection", "keep-alive");
	stream = ws::open(http_client, std::move(conflicting), error);
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::errc::invalid_upgrade));
	LIBGS_TEST_CHECK(not stream.is_open());

	bool callback_completed = false;
	ws::open_diagnostics async_diagnostics;
	ws::connect_request async_expired("https://example.test/socket");
	async_expired.handshake_timeout = std::chrono::milliseconds::zero();
	ws::open(http_client, std::move(async_expired), async_diagnostics,
		[&](libgs::error_code callback_error, ws::stream callback_stream)
		{
			LIBGS_TEST_CHECK_EQ(callback_error,
				libgs::error_code(asio::error::timed_out));
			LIBGS_TEST_CHECK(not callback_stream.is_open());
			LIBGS_TEST_CHECK_EQ(async_diagnostics.endpoint.protocol(), "wss");
			callback_completed = true;
		});
	LIBGS_TEST_CHECK(not callback_completed);
	context.run();
	LIBGS_TEST_CHECK(callback_completed);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"client preflight sync", client_preflight_sync},
		{"mixed HTTP upgrade round trip", mixed_http_upgrade_round_trip},
	});
}
