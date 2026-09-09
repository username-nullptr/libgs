// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/server.h>
#include <iostream>
#include <format>

int main(int argc, const char *argv[])
{
	const auto port = static_cast<std::uint16_t>(
		argc > 1 ? std::stoul(argv[1]) : 8080
	);
	constexpr std::string_view root_body =
		"LibGS HTTP example\nTry GET /hello/your-name\n";

	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	libgs::http::server server(std::move(acceptor));

	server
	.bind({libgs::ip_type::v4, port})
	.on_request<libgs::http::method::get>(
		"/",
		[root_body](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			co_await context.response().write (
				asio::buffer(root_body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.on_request<libgs::http::method::get>(
		"/hello/{name}",
		[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			auto &request = context.request();
			std::cout << libgs::http::method::string(request.method())
				<< ' ' << request.path() << '\n';

			const auto body = std::format (
				"Hello, {}!\n", request.path_arg("name")
			);
			co_await context.response().write (
				asio::buffer(body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.on_request<libgs::http::method::get>(
		"/cookies/set",
		[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			libgs::http::cookie cookie("stored");
			cookie.set_path("/");

			context.response().set_cookie("libgs-example", std::move(cookie));
			constexpr std::string_view body = "Cookie stored\n";

			co_await context.response().write (
				asio::buffer(body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.on_request<libgs::http::method::get>(
		"/cookies/show",
		[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			auto cookie = context.request().cookie("libgs-example");
			const auto body = cookie
				? std::format("Cookie received: {}\n", cookie->to_string())
				: std::string("Cookie missing\n");

			co_await context.response().write (
				asio::buffer(body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.on_default([](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
		constexpr std::string_view body = "Not found\n";
		context.response().set_status(libgs::http::status::not_found);

		co_await context.response().write (
			asio::buffer(body), libgs::use_awaitable
		);
		co_return;
	})
	.on_server_error([](std::error_code error)
	{
		std::cerr << "Server error: " << error.message() << '\n';
		return true;
	})
	.start();

	std::cout << "Listening on http://127.0.0.1:" << port << '\n';
	return libgs::exec();
}
