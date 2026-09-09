// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/http/client.h>
#include <libgs/http/server.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace
{

#ifdef NDEBUG
constexpr size_t warmup_count = 100;
constexpr size_t request_count = 2'000;
#else
constexpr size_t warmup_count = 10;
constexpr size_t request_count = 200;
#endif

void http_loopback_throughput()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	asio::ip::tcp::acceptor acceptor(context);
	server service(std::move(acceptor));
	auto server_config = service.config();
	server_config.keepalive_time = std::chrono::seconds(30);
	service.set_config(server_config);
	service
		.bind({libgs::ip_type::v4, 0})
		.on_request<method::get>("/benchmark",
		[](server::context_t &request_context) -> libgs::awaitable<void>
		{
			constexpr std::string_view body = "ok";
			co_await request_context.response().write(
				asio::buffer(body), libgs::use_awaitable
			);
		})
		.start();

	const auto port = service.acceptor_wrap().acceptor().local_endpoint().port();
	LIBGS_TEST_CHECK(port != 0);
	const auto url = std::format("http://127.0.0.1:{}/benchmark", port);
	client requester(context.get_executor());
	auto completed = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		try
		{
			constexpr size_t sample_count = 3;
			std::array<std::chrono::steady_clock::duration,sample_count> samples {};
			for(size_t sample = 0; sample <= sample_count; ++sample)
			{
				const auto iterations = sample == 0 ? warmup_count : request_count;
				const auto begin = std::chrono::steady_clock::now();
				for(size_t index = 0; index < iterations; ++index)
				{
					auto request = co_await requester.request_get(url, libgs::use_awaitable);
					LIBGS_TEST_CHECK(request);
					LIBGS_TEST_CHECK_EQ(
						co_await request->wait_reply(libgs::use_awaitable), status::ok
					);
					LIBGS_TEST_CHECK_EQ(
						co_await request->reply()->read<std::string>(libgs::use_awaitable), "ok"
					);
				}
				if(sample != 0)
					samples[sample - 1] = std::chrono::steady_clock::now() - begin;
			}
			std::ranges::sort(samples);
			libgs::test::print_performance_result(
				"HTTP/1.1 loopback sequential request (median of 3)", request_count,
				samples[sample_count / 2], "request"
			);

			client::req_info close_request(url);
			close_request.arg.set_header(header::connection, "close");
			auto closing = co_await requester.request_get(
				std::move(close_request), libgs::use_awaitable
			);
			LIBGS_TEST_CHECK(closing);
			LIBGS_TEST_CHECK_EQ(
				co_await closing->wait_reply(libgs::use_awaitable), status::ok
			);
			LIBGS_TEST_CHECK_EQ(
				co_await closing->reply()->read<std::string>(libgs::use_awaitable), "ok"
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
		{"HTTP loopback throughput", http_loopback_throughput},
	});
}
