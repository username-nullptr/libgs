// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/server.h>
#include <iostream>
#include <format>

int main(int argc, const char *argv[])
{
	if(argc < 2)
	{
		std::cerr << "Usage: server_file <download-file> [port] [upload-file]\n";
		return 2;
	}
	const auto download_file = std::filesystem::absolute(argv[1]);
	const auto resource_root = download_file.parent_path();
	const auto download_name = download_file.filename();

	const auto port = static_cast<std::uint16_t>(
		argc > 2 ? std::stoul(argv[2]) : 8083
	);
	const std::filesystem::path upload_file = argc > 3 ?
		argv[3] : "libgs-uploaded.bin";

	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	libgs::http::server server(std::move(acceptor));

	auto config = server.config();
	config.resource_root = resource_root;
	server.set_config(config);

	server
	.bind({libgs::ip_type::v4, port})
	.on_request<libgs::http::method::get>(
		"/download",
		[download_name](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			co_await context.response().send_file (
				download_name, libgs::use_awaitable
			);
			co_return;
		}
	)
	.on_request<libgs::http::method::put>(
		"/upload",
		[upload_file](libgs::http::server::context_t &context) -> libgs::awaitable<void>
		{
			auto bytes = co_await context.request().save_file (
				upload_file, libgs::use_awaitable
			);
			const auto body = std::format("Uploaded {} bytes\n", bytes);
			co_await context.response().write (
				asio::buffer(body), libgs::use_awaitable
			);
			co_return;
		}
	)
	.start();

	std::cout << "Resource root: " << resource_root << '\n'
		<< "Download: http://127.0.0.1:" << port << "/download\n"
		<< "Uploads: " << upload_file << '\n';

	return libgs::exec();
}
