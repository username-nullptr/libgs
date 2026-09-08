// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/parser.h>
#include <iostream>
#include <string>

int main()
{
	libgs::http::request_arg arguments;
	arguments.set_header("Accept", "text/plain");

	libgs::http::client_generator generator (
		libgs::url("http://example.test/hello?name=LibGS"),
		arguments
	);
	std::cout << "Generated request:\n"
		<< generator.header_data<libgs::http::method::get>() << '\n';

	const std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 17\r\n"
		"Connection: close\r\n"
		"\r\n"
		"Hello from LibGS\n";

	libgs::http::client_parser parser;
	auto complete = parser.append(asio::buffer(response));

	if(not complete)
	{
		std::cerr << "Parse failed: " << complete.error().message() << '\n';
		return 1;
	}
	if(not *complete)
	{
		std::cerr << "The response was unexpectedly incomplete\n";
		return 1;
	}
	std::cout << "Parsed status: " << parser.status() << '\n';
	std::cout << "Parsed body: " << parser.take_body();
	return 0;
}
