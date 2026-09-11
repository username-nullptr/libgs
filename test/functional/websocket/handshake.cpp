// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/websocket/client.h>
#include <libgs/websocket/server.h>

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
			options.response_headers[libgs::http::header::connection] = "close";
			options.response_headers[libgs::http::header::upgrade] = "not-websocket";
			options.response_headers["Sec-WebSocket-Accept"] = "not-the-accept-key";
			options.origin_validator = [](libgs::optional<std::string_view> origin)
				-> ws::upgrade_validation_result
			{
				if( origin and *origin == "https://example.test" )
					return libgs::nullopt;
				return ws::upgrade_rejection {
					.status = libgs::http::status::forbidden,
					.body = "origin rejected"
				};
			};
			auto [upgrade_error, accepted] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			if( upgrade_error )
				co_return;
			auto message = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			if( message.type != ws::message_type::text or message.body != "hello" )
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
			if( error != ws::errc::handshake_rejected )
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
			LIBGS_TEST_CHECK(diagnostics.reply->contains_header(
				libgs::http::header::connection, "Upgrade"));
			LIBGS_TEST_CHECK(diagnostics.reply->contains_header(
				libgs::http::header::upgrade, "websocket"));
			LIBGS_TEST_CHECK(diagnostics.reply->header("Sec-WebSocket-Accept")
				->to_string() != "not-the-accept-key");
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

void cross_origin_redirect_credentials()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor redirect_acceptor(context);
	asio::ip::tcp::acceptor destination_acceptor(context);
	libgs::http::server redirect_service(std::move(redirect_acceptor));
	libgs::http::server destination_service(std::move(destination_acceptor));
	for( auto *service : {&redirect_service, &destination_service} )
	{
		auto config = service->config();
		config.keepalive_time = std::chrono::milliseconds(20);
		service->set_config(config);
	}

	bool destination_checked = false;
	bool destination_authorization_present = false;
	bool destination_explicit_cookie_present = false;
	std::string destination_cookie_value;
	destination_service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/seed",
		[](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			http_context.response().set_cookie(
				"destination", libgs::http::cookie("stored").set_path("/"));
			constexpr std::string_view body = "seeded";
			co_await http_context.response().write(
				asio::buffer(body), libgs::use_awaitable);
		})
		.on_request<libgs::http::method::get>("/socket",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			auto &http_request = http_context.request();
			destination_authorization_present = http_request.contains_header(
				libgs::http::header::authorization);
			destination_explicit_cookie_present =
				http_request.contains_cookie("explicit");
			destination_cookie_value = http_request.cookie("destination")
				.value_or("").to_string();
			ws::upgrade_options options;
			options.request_validator = [&](const ws::request_info&)
				-> ws::upgrade_validation_result
			{
				destination_checked = true;
				return libgs::nullopt;
			};
			auto [upgrade_error, accepted] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			if( upgrade_error )
				co_return;
			auto message = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			co_await accepted.stream.write_text(
				message.body, libgs::use_awaitable);
			auto [close_error, trailing] = co_await
				accepted.stream.read<std::string>(
					asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(close_error, trailing);
		})
		.start();

	const auto destination_port = destination_service.acceptor_wrap()
		.acceptor().local_endpoint().port();
	const auto destination_http = std::format(
		"http://127.0.0.1:{}", destination_port);
	const auto destination_ws = std::format(
		"ws://127.0.0.1:{}/socket", destination_port);

	redirect_service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/start",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			http_context.response()
				.set_status(libgs::http::status::temporary_redirect)
				.set_header(libgs::http::header::location, destination_ws);
			co_await http_context.response().write(libgs::use_awaitable);
		})
		.start();
	const auto redirect_port = redirect_service.acceptor_wrap()
		.acceptor().local_endpoint().port();

	libgs::http::client http_client(context.get_executor());
	auto completed = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		try
		{
			auto seed = co_await http_client.request_get(
				destination_http + "/seed", libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(co_await seed->wait_reply(libgs::use_awaitable),
				libgs::http::status::ok);
			LIBGS_TEST_CHECK_EQ(co_await seed->reply()->read<std::string>(
				libgs::use_awaitable), "seeded");

			ws::connect_request request(std::format(
				"ws://127.0.0.1:{}/start", redirect_port));
			request.max_redirects = 1;
			request.request_options
				.set_basic_auth("user", "password")
				.set_cookie("explicit", "source");
			auto stream = co_await ws::open(
				http_client, std::move(request), libgs::use_awaitable);
			co_await stream.write_text("redirected", libgs::use_awaitable);
			auto reply = co_await stream.read<std::string>(libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(reply.body, "redirected");
			auto closed = co_await stream.close(libgs::use_awaitable);
			LIBGS_TEST_CHECK(closed.clean);
			LIBGS_TEST_CHECK(destination_checked);
			LIBGS_TEST_CHECK(not destination_authorization_present);
			LIBGS_TEST_CHECK(not destination_explicit_cookie_present);
			LIBGS_TEST_CHECK_EQ(destination_cookie_value, "stored");
		}
		catch(...)
		{
			redirect_service.stop();
			destination_service.stop();
			throw;
		}
		redirect_service.stop();
		destination_service.stop();
	}, asio::use_future);
	context.run();
	completed.get();
}

void validator_failures_are_bounded()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	libgs::http::server service(std::move(acceptor));
	auto service_config = service.config();
	service_config.keepalive_time = std::chrono::milliseconds(20);
	service.set_config(service_config);
	libgs::error_code exception_result;
	libgs::error_code invalid_rejection_result;
	uint16_t exception_remote_port = 0;
	uint16_t invalid_rejection_remote_port = 0;

	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/throw",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.request_validator = [&](const ws::request_info &request)
				-> ws::upgrade_validation_result
			{
				exception_remote_port = request.remote_endpoint.port;
				throw std::runtime_error("validator failed");
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			exception_result = error;
		})
		.on_request<libgs::http::method::get>("/invalid-rejection",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.request_validator = [&](const ws::request_info &request)
				-> ws::upgrade_validation_result
			{
				invalid_rejection_remote_port = request.remote_endpoint.port;
				return ws::upgrade_rejection {
					.status = libgs::http::status::switching_protocols,
					.headers = {
						{libgs::http::header::content_length, "999"},
						{libgs::http::header::transfer_encoding, "chunked"},
					},
					.body = "bounded rejection",
				};
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			invalid_rejection_result = error;
		})
		.start();

	const auto port = service.acceptor_wrap().acceptor().local_endpoint().port();
	const auto base = std::format("ws://127.0.0.1:{}", port);
	ws::client client(context.get_executor());
	auto completed = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			try
			{
				ws::open_diagnostics diagnostics;
				auto [throw_error, throw_stream] = co_await client.open(
					ws::connect_request(base + "/throw"), diagnostics,
					asio::as_tuple(libgs::use_awaitable));
				LIBGS_TEST_CHECK_EQ(throw_error,
					ws::make_error_code(ws::errc::handshake_rejected));
				LIBGS_TEST_CHECK(not throw_stream.is_open());
				LIBGS_TEST_CHECK(diagnostics.reply);
				LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
					libgs::http::status::internal_server_error);
				libgs::ignore_unused(co_await diagnostics.reply->read<std::string>(
					libgs::use_awaitable));

				auto [reject_error, reject_stream] = co_await client.open(
					ws::connect_request(base + "/invalid-rejection"), diagnostics,
					asio::as_tuple(libgs::use_awaitable));
				LIBGS_TEST_CHECK_EQ(reject_error,
					ws::make_error_code(ws::errc::handshake_rejected));
				LIBGS_TEST_CHECK(not reject_stream.is_open());
				LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
					libgs::http::status::internal_server_error);
				LIBGS_TEST_CHECK_EQ(co_await diagnostics.reply->read<std::string>(
					libgs::use_awaitable), "bounded rejection");
			}
			catch(...)
			{
				service.stop();
				throw;
			}
			service.stop();
		}, asio::use_future);
	context.run();
	completed.get();
	LIBGS_TEST_CHECK_EQ(exception_result,
		std::make_error_code(std::errc::io_error));
	LIBGS_TEST_CHECK_EQ(invalid_rejection_result,
		ws::make_error_code(ws::errc::handshake_rejected));
	LIBGS_TEST_CHECK(exception_remote_port != 0);
	LIBGS_TEST_CHECK_EQ(invalid_rejection_remote_port, exception_remote_port);
}

void asynchronous_upgrade_validators()
{
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	libgs::http::server service(std::move(acceptor));
	auto service_config = service.config();
	service_config.keepalive_time = std::chrono::milliseconds(20);
	service.set_config(service_config);

	bool request_checked = false;
	bool origin_checked = false;
	bool rejected_selector_called = false;
	bool timed_validator_cancelled = false;
	libgs::error_code denied_result;
	libgs::error_code exception_result;
	libgs::error_code timeout_result;
	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/allow",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.async_request_validator =
				[&](const ws::request_info &request)
					-> libgs::awaitable<ws::upgrade_validation_result>
			{
				co_await asio::post(libgs::use_awaitable);
				request_checked = request.path == "/allow";
				co_return libgs::nullopt;
			};
			options.async_origin_validator =
				[&](libgs::optional<std::string> origin)
					-> libgs::awaitable<ws::upgrade_validation_result>
			{
				co_await asio::post(libgs::use_awaitable);
				origin_checked = origin and *origin == "https://async.example";
				co_return libgs::nullopt;
			};
			auto [error, accepted] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			if( error )
				co_return;
			auto message = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			co_await accepted.stream.write_text(message.body,
				libgs::use_awaitable);
			libgs::ignore_unused(co_await accepted.stream.close(
				libgs::use_awaitable));
		})
		.on_request<libgs::http::method::get>("/deny",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.subprotocol_selector =
				[&](std::span<const std::string>) -> libgs::optional<std::string>
			{
				rejected_selector_called = true;
				return libgs::nullopt;
			};
			options.async_request_validator =
				[](const ws::request_info&)
					-> libgs::awaitable<ws::upgrade_validation_result>
			{
				co_await asio::post(libgs::use_awaitable);
				co_return ws::upgrade_rejection {
					.status = libgs::http::status::unauthorized,
					.body = "async denied"
				};
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			denied_result = error;
		})
		.on_request<libgs::http::method::get>("/throw-async",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.async_origin_validator =
				[](libgs::optional<std::string>)
					-> libgs::awaitable<ws::upgrade_validation_result>
			{
				co_await asio::post(libgs::use_awaitable);
				throw std::runtime_error("async validator failed");
				co_return libgs::nullopt;
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			exception_result = error;
		})
		.on_request<libgs::http::method::get>("/timeout-async",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.handshake_timeout = std::chrono::milliseconds(25);
			options.async_request_validator =
				[&](const ws::request_info&)
					-> libgs::awaitable<ws::upgrade_validation_result>
			{
				asio::steady_timer timer(co_await asio::this_coro::executor);
				timer.expires_after(std::chrono::seconds(1));
				auto [error] = co_await timer.async_wait(
					asio::as_tuple(libgs::use_awaitable));
				timed_validator_cancelled = error == asio::error::operation_aborted;
				co_return libgs::nullopt;
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			timeout_result = error;
		})
		.start();

	const auto port = service.acceptor_wrap().acceptor().local_endpoint().port();
	const auto base = std::format("ws://127.0.0.1:{}", port);
	ws::client client(context.get_executor());
	auto completed = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		try
		{
			ws::connect_request allowed(base + "/allow");
			allowed.request_options.set_header(
				libgs::http::header::origin, "https://async.example");
			auto stream = co_await client.open(
				std::move(allowed), libgs::use_awaitable);
			co_await stream.write_text("authorized", libgs::use_awaitable);
			auto echoed = co_await stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(echoed.body, "authorized");
			auto closed = co_await stream.close(libgs::use_awaitable);
			LIBGS_TEST_CHECK(closed.clean);

			ws::open_diagnostics diagnostics;
			auto [deny_error, denied] = co_await client.open(
				ws::connect_request(base + "/deny"), diagnostics,
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(deny_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not denied.is_open());
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
				libgs::http::status::unauthorized);
			LIBGS_TEST_CHECK_EQ(co_await diagnostics.reply->read<std::string>(
				libgs::use_awaitable), "async denied");

			auto [throw_error, thrown] = co_await client.open(
				ws::connect_request(base + "/throw-async"), diagnostics,
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(throw_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not thrown.is_open());
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
				libgs::http::status::internal_server_error);
			libgs::ignore_unused(co_await diagnostics.reply->read<std::string>(
				libgs::use_awaitable));

			ws::connect_request timed(base + "/timeout-async");
			timed.handshake_timeout = std::chrono::milliseconds(250);
			auto [timeout_error, timed_stream] = co_await client.open(
				std::move(timed), asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK(timeout_error);
			LIBGS_TEST_CHECK(not timed_stream.is_open());

			// The failed upgrade must not poison the owned HTTP pool. A second
			// connection to the same origin still completes a full handshake.
			ws::connect_request retried(base + "/allow");
			retried.request_options.set_header(
				libgs::http::header::origin, "https://async.example");
			auto retry_stream = co_await client.open(
				std::move(retried), libgs::use_awaitable);
			co_await retry_stream.write_text("after-timeout", libgs::use_awaitable);
			auto retry_echo = co_await retry_stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(retry_echo.body, "after-timeout");
			libgs::ignore_unused(co_await retry_stream.close(libgs::use_awaitable));
		}
		catch(...)
		{
			service.stop();
			throw;
		}
		service.stop();
	}, asio::use_future);
	context.run();
	completed.get();
	LIBGS_TEST_CHECK(request_checked);
	LIBGS_TEST_CHECK(origin_checked);
	LIBGS_TEST_CHECK(not rejected_selector_called);
	LIBGS_TEST_CHECK_EQ(denied_result,
		ws::make_error_code(ws::errc::handshake_rejected));
	LIBGS_TEST_CHECK_EQ(exception_result,
		std::make_error_code(std::errc::io_error));
	LIBGS_TEST_CHECK(timed_validator_cancelled);
	LIBGS_TEST_CHECK_EQ(timeout_result,
		libgs::error_code(asio::error::timed_out));
}

} //namespace

int main()
{
	return libgs::test::run({
		{"client preflight sync", client_preflight_sync},
		{"mixed HTTP upgrade round trip", mixed_http_upgrade_round_trip},
		{"cross-origin redirect credentials", cross_origin_redirect_credentials},
		{"validator failures are bounded", validator_failures_are_bounded},
		{"asynchronous upgrade validators", asynchronous_upgrade_validators},
	});
}
