// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/websocket/client.h>
#include <libgs/websocket/server.h>
#include <libgs/websocket/detail/permessage_deflate.h>
#include <libgs/http/utils/tcp_connection.h>
#include <libgs/core/system/app_utls.h>

namespace
{

namespace ws = libgs::websocket;

class scoped_environment
{
	struct entry
	{
		std::string name;
		libgs::optional<std::string> value;
	};

public:
	scoped_environment(std::initializer_list<std::string_view> names)
	{
		for(const auto name : names)
		{
			auto value = libgs::app::getenv(name);
			m_entries.push_back({std::string(name), value ?
				libgs::optional<std::string>(*value) : libgs::nullopt});
		}
	}

	~scoped_environment()
	{
		for(const auto &item : m_entries)
		{
			if( item.value )
				libgs::ignore_unused(libgs::app::setenv(item.name, *item.value));
			else
				libgs::ignore_unused(libgs::app::unsetenv(item.name));
		}
	}

private:
	std::vector<entry> m_entries;
};

void permessage_deflate_negotiation()
{
	ws::permessage_deflate_options offer_options;
	offer_options.server_max_window_bits = 12;
	offer_options.offer_client_max_window_bits = true;
	auto offer = ws::permessage_deflate_extension(offer_options);

#if !LIBGS_WEBSOCKET_ZLIB_SUPPORT
	LIBGS_TEST_CHECK(not ws::detail::supported_extension_offers(
		std::span(&offer, 1)));
	return;
#else

	ws::permessage_deflate_options policy_options;
	policy_options.server_max_window_bits = 11;
	policy_options.client_max_window_bits = 10;
	auto policy = ws::permessage_deflate_extension(policy_options);
	auto selected = ws::detail::negotiate_permessage_deflate(offer, policy);
	LIBGS_TEST_CHECK(selected.has_value());
	LIBGS_TEST_CHECK(ws::detail::supported_extension_response(
		std::span(&*selected, 1), std::span(&offer, 1)));

	auto server_bits = std::ranges::find_if(selected->parameters,
		[](const ws::extension_parameter &parameter) {
			return parameter.name == "server_max_window_bits";
		});
	auto client_bits = std::ranges::find_if(selected->parameters,
		[](const ws::extension_parameter &parameter) {
			return parameter.name == "client_max_window_bits";
		});
	LIBGS_TEST_CHECK(server_bits != selected->parameters.end());
	LIBGS_TEST_CHECK(client_bits != selected->parameters.end());
	LIBGS_TEST_CHECK_EQ(server_bits->value.value_or(""), "11");
	LIBGS_TEST_CHECK_EQ(client_bits->value.value_or(""), "10");

	auto no_window_offer = ws::permessage_deflate_extension(
		ws::permessage_deflate_options{});
	LIBGS_TEST_CHECK(not ws::detail::supported_extension_response(
		std::span(&*selected, 1), std::span(&no_window_offer, 1)));
	auto invalid = offer;
	invalid.parameters.push_back(invalid.parameters.front());
	LIBGS_TEST_CHECK(not ws::is_permessage_deflate_extension(invalid));
#endif
}

std::string websocket_key_from_header(std::string_view header)
{
	constexpr std::string_view name = "Sec-WebSocket-Key:";
	auto position = header.find(name);
	LIBGS_TEST_CHECK(position != std::string_view::npos);
	position += name.size();
	while( position < header.size() and
		(header[position] == ' ' or header[position] == '\t') )
		++position;
	auto end = header.find("\r\n", position);
	LIBGS_TEST_CHECK(end != std::string_view::npos);
	return std::string(header.substr(position, end - position));
}

libgs::awaitable<void> proxy_websocket_echo(asio::ip::tcp::socket socket,
	std::string pending = {})
{
	while( pending.find("\r\n\r\n") == std::string::npos )
	{
		std::array<char,1024> input {};
		auto size = co_await socket.async_read_some(asio::buffer(input),
			libgs::use_awaitable);
		pending.append(input.data(), size);
	}
	LIBGS_TEST_CHECK(pending.starts_with("GET /socks HTTP/1.1\r\n"));
	auto accept = ws::make_accept_key(websocket_key_from_header(pending));
	LIBGS_TEST_CHECK(accept.has_value());
	const auto response = std::format(
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: {}\r\n\r\n", *accept);
	co_await asio::async_write(socket, asio::buffer(response), libgs::use_awaitable);

	auto transport = std::make_shared<libgs::http::tcp_connection>(std::move(socket));
	ws::stream stream(transport->get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(transport),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto message = co_await stream.read<std::string>(libgs::use_awaitable);
	LIBGS_TEST_CHECK_EQ(message.body, "through-socks5");
	co_await stream.write_text("socks5-echo", libgs::use_awaitable);
	stream.shutdown();
}

void proxy_round_trips()
{
	const scoped_environment environment({
		"ws_proxy", "WS_PROXY", "no_proxy", "NO_PROXY",
	});
	libgs::io_context_t context;
	bool forward_authenticated = false;
	asio::ip::tcp::acceptor forward_acceptor(context);
	libgs::http::server forward_proxy(std::move(forward_acceptor));
	forward_proxy.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/forward",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			auto authorization = http_context.request().header(
				libgs::http::header::proxy_authorization);
			forward_authenticated = authorization and
				authorization->to_string() == "Basic dXNlcjpzZWNyZXQ=" and
				http_context.request().target().starts_with(
					"http://origin.example/forward");
			ws::upgrade_options options;
			auto accepted = co_await ws::upgrade(http_context,
				std::move(options), libgs::use_awaitable);
			auto message = co_await accepted.stream.read<std::string>(
				libgs::use_awaitable);
			co_await accepted.stream.write_text("forward-" + message.body,
				libgs::use_awaitable);
			accepted.stream.shutdown();
		}).start();
	const auto forward_port = forward_proxy.acceptor_wrap()
		.acceptor().local_endpoint().port();
	LIBGS_TEST_CHECK(libgs::app::setenv("ws_proxy", std::format(
		"http://user:secret@127.0.0.1:{}/", forward_port)));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("WS_PROXY"));
#endif
	LIBGS_TEST_CHECK(libgs::app::unsetenv("no_proxy"));
	LIBGS_TEST_CHECK(libgs::app::unsetenv("NO_PROXY"));

	asio::ip::tcp::acceptor socks_acceptor(context,
		{asio::ip::address_v4::loopback(), 0});
	const auto socks_port = socks_acceptor.local_endpoint().port();
	auto socks_server = asio::co_spawn(context,
		[acceptor = std::move(socks_acceptor)]() mutable -> libgs::awaitable<void>
		{
			auto socket = co_await acceptor.async_accept(libgs::use_awaitable);
			std::array<uint8_t,2> greeting {};
			co_await asio::async_read(socket, asio::buffer(greeting),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(greeting[0], uint8_t {5});
			std::vector<uint8_t> methods(greeting[1]);
			co_await asio::async_read(socket, asio::buffer(methods),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK(std::ranges::find(methods, uint8_t {2}) != methods.end());
			const std::array<uint8_t,2> select_auth {5, 2};
			co_await asio::async_write(socket, asio::buffer(select_auth),
				libgs::use_awaitable);

			std::array<uint8_t,2> auth_header {};
			co_await asio::async_read(socket, asio::buffer(auth_header),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(auth_header[0], uint8_t {1});
			std::vector<char> username(auth_header[1]);
			co_await asio::async_read(socket, asio::buffer(username),
				libgs::use_awaitable);
			uint8_t password_size = 0;
			co_await asio::async_read(socket, asio::buffer(&password_size, 1),
				libgs::use_awaitable);
			std::vector<char> password(password_size);
			co_await asio::async_read(socket, asio::buffer(password),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(std::string(username.begin(), username.end()), "user");
			LIBGS_TEST_CHECK_EQ(std::string(password.begin(), password.end()), "secret");
			const std::array<uint8_t,2> auth_ok {1, 0};
			co_await asio::async_write(socket, asio::buffer(auth_ok),
				libgs::use_awaitable);

			std::array<uint8_t,5> request {};
			co_await asio::async_read(socket, asio::buffer(request),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(request[0], uint8_t {5});
			LIBGS_TEST_CHECK_EQ(request[1], uint8_t {1});
			LIBGS_TEST_CHECK_EQ(request[3], uint8_t {3});
			std::vector<char> host(request[4]);
			co_await asio::async_read(socket, asio::buffer(host),
				libgs::use_awaitable);
			std::array<uint8_t,2> port {};
			co_await asio::async_read(socket, asio::buffer(port),
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(std::string(host.begin(), host.end()), "origin.example");
			LIBGS_TEST_CHECK_EQ((static_cast<uint16_t>(port[0]) << 8) | port[1],
				uint16_t {80});
			const std::array<uint8_t,10> connected {5, 0, 0, 1, 127, 0, 0, 1, 0, 0};
			co_await asio::async_write(socket, asio::buffer(connected),
				libgs::use_awaitable);
			co_await proxy_websocket_echo(std::move(socket));
		}, asio::use_future);

	ws::client client(context.get_executor());
	auto clients = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			ws::connect_request forward("ws://origin.example/forward");
			LIBGS_TEST_CHECK(not forward.proxy);
			auto forward_stream = co_await client.open(
				std::move(forward), libgs::use_awaitable);
			co_await forward_stream.write_text("proxy", libgs::use_awaitable);
			auto forward_reply = co_await forward_stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(forward_reply.body, "forward-proxy");
			forward_stream.shutdown();

			ws::connect_request socks("ws://origin.example/socks");
			ws::proxy_config socks_proxy {
				.type = ws::proxy_type::socks5,
				.endpoint = libgs::url(std::format(
					"socks5://127.0.0.1:{}", socks_port)),
			};
			socks_proxy.set_basic_auth("user", "secret");
			socks.proxy = std::move(socks_proxy);
			auto socks_stream = co_await client.open(
				std::move(socks), libgs::use_awaitable);
			co_await socks_stream.write_text("through-socks5", libgs::use_awaitable);
			auto socks_reply = co_await socks_stream.read<std::string>(
				libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(socks_reply.body, "socks5-echo");
			socks_stream.shutdown();
			forward_proxy.stop();
		}, asio::use_future);

	context.run();
	clients.get();
	socks_server.get();
	LIBGS_TEST_CHECK(forward_authenticated);
}

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
	bool async_subprotocol_checked = false;
	bool async_extension_checked = false;
	size_t async_subprotocol_calls = 0;
	size_t async_extension_calls = 0;
	bool rejected_selector_called = false;
	bool conflicting_selector_called = false;
	bool sync_async_selector_called = false;
	bool timed_validator_cancelled = false;
	libgs::error_code denied_result;
	libgs::error_code exception_result;
	libgs::error_code selector_exception_result;
	libgs::error_code invalid_selector_result;
	libgs::error_code selector_conflict_result;
	libgs::error_code sync_async_selector_result;
	libgs::error_code timeout_result;
	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<libgs::http::method::get>("/allow",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.supported_subprotocols = {"async.v1", "async.v2"};
			options.require_subprotocol = true;
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
			auto extension_policy = ws::permessage_deflate_extension();
			options.supported_extensions = {extension_policy};
#endif
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
			options.async_subprotocol_selector =
				[&](const ws::request_info &request,
					std::span<const std::string> offered)
					-> libgs::awaitable<libgs::optional<std::string>>
			{
				co_await asio::post(libgs::use_awaitable);
				++async_subprotocol_calls;
				async_subprotocol_checked = request.path == "/allow" and
					request.request_headers.contains("X-Tenant") and
					offered.size() == 2 and offered[0] == "async.v1" and
					offered[1] == "async.v2";
				co_return std::string("async.v2");
			};
			options.async_extension_selector =
				[&
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
					, extension_policy = std::move(extension_policy)
#endif
				](const ws::request_info &request,
					std::span<const ws::extension> offered)
					-> libgs::awaitable<std::vector<ws::extension>>
			{
				co_await asio::post(libgs::use_awaitable);
				++async_extension_calls;
				async_extension_checked = request.target == "/allow" and
					request.request_headers.at("X-Tenant").to_string() == "gold";
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
				async_extension_checked = async_extension_checked and offered.size() == 1;
				if( offered.size() == 1 )
				{
					auto selected = ws::detail::negotiate_permessage_deflate(
						offered.front(), extension_policy);
					if( selected )
						co_return std::vector<ws::extension>{std::move(*selected)};
				}
#else
				async_extension_checked = async_extension_checked and offered.empty();
#endif
				co_return std::vector<ws::extension>{};
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
				[&](const ws::request_info&, std::span<const std::string>)
					-> libgs::optional<std::string>
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
		.on_request<libgs::http::method::get>("/invalid-selector",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.supported_subprotocols = {"selector.v1"};
			options.async_subprotocol_selector =
				[](const ws::request_info&, std::span<const std::string>)
					-> libgs::awaitable<libgs::optional<std::string>>
			{
				co_return std::string("selector.v2");
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			invalid_selector_result = error;
		})
		.on_request<libgs::http::method::get>("/throw-selector",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.supported_subprotocols = {"selector.v1"};
			options.async_subprotocol_selector =
				[](const ws::request_info&, std::span<const std::string>)
					-> libgs::awaitable<libgs::optional<std::string>>
			{
				co_await asio::post(libgs::use_awaitable);
				throw std::runtime_error("async selector failed");
				co_return libgs::nullopt;
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			selector_exception_result = error;
		})
		.on_request<libgs::http::method::get>("/selector-conflict",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.subprotocol_selector =
				[&](const ws::request_info&, std::span<const std::string>)
					-> libgs::optional<std::string>
			{
				conflicting_selector_called = true;
				return libgs::nullopt;
			};
			options.async_subprotocol_selector =
				[&](const ws::request_info&, std::span<const std::string>)
					-> libgs::awaitable<libgs::optional<std::string>>
			{
				conflicting_selector_called = true;
				co_return libgs::nullopt;
			};
			auto [error, result] = co_await ws::upgrade(
				http_context, std::move(options),
				asio::as_tuple(libgs::use_awaitable));
			libgs::ignore_unused(result);
			selector_conflict_result = error;
		})
		.on_request<libgs::http::method::get>("/sync-async-selector",
		[&](libgs::http::server::context_t &http_context) -> libgs::awaitable<void>
		{
			ws::upgrade_options options;
			options.async_subprotocol_selector =
				[&](const ws::request_info&, std::span<const std::string>)
					-> libgs::awaitable<libgs::optional<std::string>>
			{
				sync_async_selector_called = true;
				co_return libgs::nullopt;
			};
			auto result = ws::upgrade(http_context, std::move(options),
				sync_async_selector_result);
			libgs::ignore_unused(result);
			co_return;
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
			allowed.subprotocols = {"async.v1", "async.v2"};
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
			allowed.extensions = {ws::permessage_deflate_extension()};
#endif
			allowed.request_options.set_header(
				libgs::http::header::origin, "https://async.example");
			allowed.request_options.set_header("X-Tenant", "gold");
			auto stream = co_await client.open(
				std::move(allowed), libgs::use_awaitable);
			LIBGS_TEST_CHECK_EQ(stream.negotiated_subprotocol(), "async.v2");
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
			LIBGS_TEST_CHECK_EQ(stream.negotiated_extensions().size(), 1U);
#endif
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

			ws::connect_request invalid_selector(base + "/invalid-selector");
			invalid_selector.subprotocols = {"selector.v1"};
			auto [invalid_selector_error, invalid_selector_stream] =
				co_await client.open(std::move(invalid_selector), diagnostics,
					asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(invalid_selector_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not invalid_selector_stream.is_open());
			libgs::ignore_unused(co_await diagnostics.reply->read<std::string>(
				libgs::use_awaitable));

			ws::connect_request selector_throw(base + "/throw-selector");
			selector_throw.subprotocols = {"selector.v1"};
			auto [selector_throw_error, selector_thrown] = co_await client.open(
				std::move(selector_throw), diagnostics,
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(selector_throw_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not selector_thrown.is_open());
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
				libgs::http::status::internal_server_error);
			libgs::ignore_unused(co_await diagnostics.reply->read<std::string>(
				libgs::use_awaitable));

			auto [conflict_error, conflict_stream] = co_await client.open(
				ws::connect_request(base + "/selector-conflict"), diagnostics,
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(conflict_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not conflict_stream.is_open());
			LIBGS_TEST_CHECK_EQ(diagnostics.reply->status(),
				libgs::http::status::internal_server_error);
			libgs::ignore_unused(co_await diagnostics.reply->read<std::string>(
				libgs::use_awaitable));

			auto [sync_selector_error, sync_selector_stream] = co_await client.open(
				ws::connect_request(base + "/sync-async-selector"), diagnostics,
				asio::as_tuple(libgs::use_awaitable));
			LIBGS_TEST_CHECK_EQ(sync_selector_error,
				ws::make_error_code(ws::errc::handshake_rejected));
			LIBGS_TEST_CHECK(not sync_selector_stream.is_open());
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
			retried.subprotocols = {"async.v1", "async.v2"};
#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
			retried.extensions = {ws::permessage_deflate_extension()};
#endif
			retried.request_options.set_header(
				libgs::http::header::origin, "https://async.example");
			retried.request_options.set_header("X-Tenant", "gold");
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
	LIBGS_TEST_CHECK(async_subprotocol_checked);
	LIBGS_TEST_CHECK(async_extension_checked);
	LIBGS_TEST_CHECK_EQ(async_subprotocol_calls, 2U);
	LIBGS_TEST_CHECK_EQ(async_extension_calls, 2U);
	LIBGS_TEST_CHECK(not rejected_selector_called);
	LIBGS_TEST_CHECK_EQ(denied_result,
		ws::make_error_code(ws::errc::handshake_rejected));
	LIBGS_TEST_CHECK_EQ(exception_result,
		std::make_error_code(std::errc::io_error));
	LIBGS_TEST_CHECK_EQ(selector_exception_result,
		std::make_error_code(std::errc::io_error));
	LIBGS_TEST_CHECK_EQ(invalid_selector_result,
		ws::make_error_code(ws::errc::unsupported_subprotocol));
	LIBGS_TEST_CHECK_EQ(selector_conflict_result,
		std::make_error_code(std::errc::invalid_argument));
	LIBGS_TEST_CHECK(not conflicting_selector_called);
	LIBGS_TEST_CHECK_EQ(sync_async_selector_result,
		std::make_error_code(std::errc::operation_not_supported));
	LIBGS_TEST_CHECK(not sync_async_selector_called);
	LIBGS_TEST_CHECK(timed_validator_cancelled);
	LIBGS_TEST_CHECK_EQ(timeout_result,
		libgs::error_code(asio::error::timed_out));
}

} //namespace

int main(int argc, const char *const argv[])
{
	return libgs::test::run(argc, argv, {
		{"permessage-deflate negotiation", permessage_deflate_negotiation},
		{"client preflight sync", client_preflight_sync},
		{"proxy round trips", proxy_round_trips},
		{"mixed HTTP upgrade round trip", mixed_http_upgrade_round_trip},
		{"cross-origin redirect credentials", cross_origin_redirect_credentials},
		{"validator failures are bounded", validator_failures_are_bounded},
		{"asynchronous upgrade validators", asynchronous_upgrade_validators},
	});
}
