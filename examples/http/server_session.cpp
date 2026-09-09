// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/server.h>

#include <cstdint>
#include <format>
#include <iostream>

int main(int argc, const char *argv[])
{
	const auto port = static_cast<std::uint16_t>(
		argc > 1 ? std::stoul(argv[1]) : 8082
	);
	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	libgs::http::server server(std::move(acceptor));

	server
	.bind({libgs::ip_type::v4, port})
	.on_request<libgs::http::method::get>(
		"/session",
		[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			auto session = context.session();
			session->set_attribute("example", std::string("active"));

			const auto body = std::format (
				"Session ID: {}\n", session->id()
			);
			co_await context.response().write (
				asio::buffer(body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.start();

	std::cout << "Listening on http://127.0.0.1:" << port
		<< "/session\n";
	return libgs::exec();
}
