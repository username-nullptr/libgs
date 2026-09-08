// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/client.h>
#include <iostream>
#include <string>

int main(int argc, const char *argv[])
{
	const std::string url = argc > 1 ?
		argv[1] : "http://127.0.0.1:8080/hello/LibGS";

	std::error_code error;
	libgs::http::request_arg arguments;

	arguments
	.set_header("Accept", "text/plain")
	.set_cookie("libgs-example", "1");

	libgs::http::client::req_info request(url, arguments);
	request.follow_redirects(3);

	libgs::http::client client;
	auto context = client.request_get(std::move(request), error);
	if(error)
	{
		std::cerr << "Request failed: " << error.message() << '\n';
		return 1;
	}
	auto status = context->wait_reply(error);
	if(error)
	{
		std::cerr << "Reply failed: " << error.message() << '\n';
		return 1;
	}
	auto body = context->reply()->read<std::string>(error);
	if(error)
	{
		std::cerr << "Body read failed: " << error.message() << '\n';
		return 1;
	}
	std::cout << "HTTP status: " << status << '\n';
	std::cout << body;
	return 0;
}
