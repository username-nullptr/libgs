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

int main()
{
#if 0
	libgs::http::client client;
	client.request_get("http://www.baidu.com")

	.and_then([&](const auto &context)
	{
		using context_t = std::remove_cvref_t<decltype(context)>;
		libgs::sys_expected<context_t> result;

		auto expected = context->wait_reply();
		if( expected )
			result = context;
		else
			result.despair(expected.error());
		return result;
	})
	.and_then([&](const auto &context)
	{
		auto asd = context->reply().header(libgs::http::protocol::header::content_length);
		return context->reply().read();
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

	auto asd = client.upload_file("http://www.baidu.com", "./hello.txt");

	return 0;
#else
	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		libgs::http::client client;
		try {
			auto context = *(co_await client.request_get (
				"http://www.baidu.com", libgs::use_awaitable
			)).or_else([](const auto &error) {
				libgs::system_error::loc_throw(error);
			});

			(co_await context->wait_reply(libgs::use_awaitable))
				.or_else([](const auto &error) {
					libgs::system_error::loc_throw(error);
				});

			auto body = *(co_await context->reply().read(libgs::use_awaitable))
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