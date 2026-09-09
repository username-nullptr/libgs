// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/client.h>
#include <libgs/websocket/client.h>
#include <iostream>
#include <string>

namespace ws = libgs::websocket;

int main(int argc, const char *argv[])
{
	const std::string http_url = argc > 1 ?
		argv[1] : "http://127.0.0.1:8080/mixed";
	const std::string websocket_url = argc > 2 ?
		argv[2] : "ws://127.0.0.1:8080/mixed";

	// One HTTP client owns both the ordinary request and the Upgrade request.
	libgs::http::client http_client;
	libgs::dispatch(
		[&http_client, http_url, websocket_url]() -> libgs::awaitable<void>
		{
			try
			{
				auto request = co_await http_client.request_get(
					http_url, libgs::use_awaitable);
				const auto status = co_await request->wait_reply(
					libgs::use_awaitable);
				const auto body = co_await request->reply()->read<std::string>(
					libgs::use_awaitable);
				std::cout << "HTTP " << static_cast<unsigned>(status)
					<< ": " << body;

				ws::connect_request upgrade_request(websocket_url);
				upgrade_request.subprotocols = {"libgs.example"};
				ws::open_diagnostics diagnostics;
				auto stream = co_await ws::open(
					http_client, std::move(upgrade_request), diagnostics,
					libgs::use_awaitable);

				std::cout << "Upgrade HTTP "
					<< static_cast<unsigned>(diagnostics.reply->status())
					<< ", subprotocol: " << stream.negotiated_subprotocol()
					<< '\n';
				co_await stream.write_text("hello over WebSocket",
					libgs::use_awaitable);
				auto message = co_await stream.read<std::string>(
					libgs::use_awaitable);
				std::cout << "WebSocket: " << message.body << '\n';

				const auto close_info = co_await stream.close(
					libgs::use_awaitable);
				std::cout << "WebSocket clean close: " << std::boolalpha
					<< close_info.clean << '\n';
			}
			catch(const std::exception &exception)
			{
				std::cerr << "Mixed HTTP/WebSocket client failed: "
					<< exception.what() << '\n';
				libgs::exit(1);
				co_return;
			}
			libgs::exit();
			co_return;
		}
	);
	return libgs::exec();
}
