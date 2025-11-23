// #include <libgs/http/server.h>
#include <libgs/http/client/client.h>

#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>

#include <libgs/coro/utils.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>

#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
#if 0
	libgs::http::client<> client;
	client.req_get("http://www.baidu.com")

	.and_then([](const auto &request)
	{
		// request->arg().set_header(libgs::http::protocol::header::expect, "100-continue");
		return request->write().transform([&](size_t) {
			return request;
		});
	})
	.and_then([&](const auto &request) {
		return client.reply(request);
	})
	.and_then([&](const auto &reply)
	{
		auto asd = reply->header(libgs::http::protocol::header::content_length);
		return reply->read();
	})
	.transform([](std::string_view body)
	{
		size_t sss = body.size();
		int i = 0;
		i = 11;
	})
	.or_else([](const libgs::error_code &error)
	{
		spdlog::error("--------------- {}", error);
	});
	return 0;
#else
	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		libgs::http::client<> client;
		try {
			auto request = *(co_await client.req_get (
				"http://www.baidu.com", libgs::use_awaitable
			)).or_else([](const auto &error) {
				libgs::system_error::loc_throw(error);
			});

			(co_await request->write(libgs::use_awaitable))
			.or_else([](const auto &error) {
				libgs::system_error::loc_throw(error);
			});

			auto reply = *(co_await client.reply(request, libgs::use_awaitable))
			.or_else([](const auto &error) {
				libgs::system_error::loc_throw(error);
			});

			auto body = *(co_await reply->read(libgs::use_awaitable))
			.or_else([](const auto &error) {
				libgs::system_error::loc_throw(error);
			});

			int i = 0;
			i = 11;
		}
		catch(const std::exception &ex) {
			spdlog::error("=------=-========= {}", ex);
		}
		co_return libgs::exit(0);
	});
	return libgs::exec();
#endif
}