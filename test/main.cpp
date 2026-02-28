#include <libgs/http_nt/client.h>

#include <libgs/core/system/app_utls.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>

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
	spdlog::set_level(spdlog::level::trace);

	libgs::http_nt::client client;
	libgs::http_nt::request_arg arg;

#if 0
	auto expected = client.request_get({"http://www.baidu.com", arg});
	auto context = *expected;

	auto status = context->wait_reply();
	// auto body = context->reply()->read();
	auto sum = context->reply()->save_file("./baidu.html");

	return 0;
#else
	libgs::dispatch([&]() mutable -> libgs::awaitable<void>
	{
		auto expected = co_await client.request_get (
			{"http://www.baidu.com", arg}, libgs::use_awaitable
		);
		auto context = *expected;

		auto status = co_await context->wait_reply(libgs::use_awaitable);
		// auto body = co_await context->reply()->read(libgs::use_awaitable);
		auto sum = co_await context->reply()->save_file("./baidu.html", libgs::use_awaitable);

		libgs::exit();
		co_return ;
	});
	return libgs::exec();
#endif
}
