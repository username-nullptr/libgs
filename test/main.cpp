// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>

#include <libgs/coro/utils.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>
#include <iostream>
#include <memory>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	// libgs::utils::process aaaa;

	// libgs::dispatch([&]() -> libgs::awaitable<void>
	// {
	// 	libgs::error_code error;
	// 	auto aaa = co_await aaaa.run("bbb", libgs::use_awaitable | error);
	//
	// 	spdlog::info("000000000000000 ==== {}", aaa);
	//
	// 	char buffer[1024] {0};
	// 	auto bbb = co_await aaaa.read({buffer, 1024}, libgs::use_awaitable | error);
	//
	// 	spdlog::info("1111111111 ==== {}", bbb);
	//
	// 	auto ccc = co_await aaaa.write({buffer, 111}, libgs::use_awaitable | error);
	//
	// 	spdlog::info("22222222222222 ==== {}", ccc);
	//
	// 	libgs::exit(111);
	// 	co_return ;
	// });
	// return libgs::exec();

	libgs::utils::process::exec (
		// "lshw 2>/dev/null "
		// " | grep -m 1 'serial:' "
		// " | awk '{print substr($2, length($2)-7)}' "
		"aaa | aed"
	)
	.transform([](int code)
	{
		spdlog::info("exit code: {}", code);
	})
	.or_else([](const libgs::error_code &error)
	{
		spdlog::error("process crashed: {}", error);
	});
	return 0;
}