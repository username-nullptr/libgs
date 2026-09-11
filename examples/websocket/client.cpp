// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/client.h>
#include <iostream>

namespace ws = libgs::websocket;

int main(int argc, const char *argv[])
{
	const std::string endpoint = argc > 1 ?
		argv[1] : "ws://127.0.0.1:8080/echo";

	ws::client client;
	libgs::dispatch([&client, endpoint]() -> libgs::awaitable<void>
	{
		try {
			auto stream = co_await client.open (
				ws::connect_request(endpoint), libgs::use_awaitable
			);
			co_await stream.write_text("hello", libgs::use_awaitable);
			auto message = co_await stream.read<std::string>(libgs::use_awaitable);

			std::cout << message.body << '\n';
			co_await stream.close(libgs::use_awaitable);
		}
		catch(const std::exception &exception)
		{
			std::cerr << "WebSocket client failed: " << exception.what() << '\n';
			libgs::exit(1);
			co_return ;
		}
		libgs::exit();
		co_return ;
	});
	return libgs::exec();
}
