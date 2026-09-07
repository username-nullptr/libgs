#include "test.h"

#include <libgs/http/client.h>
#include <libgs/http/server.h>

#include <format>
#include <string>

namespace
{

void client_server_round_trip()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	server service(std::move(acceptor));
	auto server_config = service.config();
	server_config.keepalive_time = std::chrono::milliseconds(20);
	service.set_config(server_config);
	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<method::get>("/hello/{name}",
		[](server::context_t &request_context) -> libgs::awaitable<void>
		{
			auto &request = request_context.request();
			const auto body = std::format(
				"hello:{}:{}",
				request.path_arg("name").value_or("missing").to_string(),
				request.parameter("value").value_or("missing").to_string()
			);
			request_context.response().set_header("X-LibGS-Test", "loopback");
			co_await request_context.response().write(asio::buffer(body), libgs::use_awaitable);
		})
		.on_request<method::get>("/cookie/set",
		[](server::context_t &request_context) -> libgs::awaitable<void>
		{
			request_context.response().set_cookie(
				"session", cookie("stored").set_path("/")
			);
			constexpr std::string_view body = "set";
			co_await request_context.response().write(asio::buffer(body), libgs::use_awaitable);
		})
		.on_request<method::get>("/cookie/show",
		[](server::context_t &request_context) -> libgs::awaitable<void>
		{
			const auto value = request_context.request().cookie("session")
				.value_or("missing").to_string();
			co_await request_context.response().write(asio::buffer(value), libgs::use_awaitable);
		})
		.on_default([](server::context_t &request_context) -> libgs::awaitable<void>
		{
			request_context.response().set_status(status::not_found);
			constexpr std::string_view body = "not found";
			co_await request_context.response().write(asio::buffer(body), libgs::use_awaitable);
		})
		.start();

	const auto port = service.acceptor_wrap().acceptor().local_endpoint().port();
	LIBGS_TEST_CHECK(port != 0);
	const auto base = std::format("http://127.0.0.1:{}", port);
	client requester(context.get_executor());
	auto completed = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		try
		{
			auto hello = co_await requester.request_get(
				base + "/hello/LibGS?value=42", libgs::use_awaitable
			);
			LIBGS_TEST_CHECK(hello);
			LIBGS_TEST_CHECK_EQ(
				co_await hello->wait_reply(libgs::use_awaitable), status::ok
			);
			LIBGS_TEST_CHECK_EQ(
				hello->reply()->header("X-LibGS-Test")->to_string(), "loopback"
			);
			LIBGS_TEST_CHECK_EQ(
				co_await hello->reply()->read<std::string>(libgs::use_awaitable),
				"hello:LibGS:42"
			);

			auto set_cookie = co_await requester.request_get(
				base + "/cookie/set", libgs::use_awaitable
			);
			LIBGS_TEST_CHECK_EQ(
				co_await set_cookie->wait_reply(libgs::use_awaitable), status::ok
			);
			LIBGS_TEST_CHECK_EQ(
				co_await set_cookie->reply()->read<std::string>(libgs::use_awaitable), "set"
			);

			auto show_cookie = co_await requester.request_get(
				base + "/cookie/show", libgs::use_awaitable
			);
			LIBGS_TEST_CHECK_EQ(
				co_await show_cookie->wait_reply(libgs::use_awaitable), status::ok
			);
			LIBGS_TEST_CHECK_EQ(
				co_await show_cookie->reply()->read<std::string>(libgs::use_awaitable), "stored"
			);

			auto missing = co_await requester.request_get(
				base + "/missing", libgs::use_awaitable
			);
			LIBGS_TEST_CHECK_EQ(
				co_await missing->wait_reply(libgs::use_awaitable), status::not_found
			);
			LIBGS_TEST_CHECK_EQ(
				co_await missing->reply()->read<std::string>(libgs::use_awaitable), "not found"
			);
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
}

} //namespace

int main()
{
	return libgs::test::run({
		{"client/server loopback", client_server_round_trip},
	});
}
