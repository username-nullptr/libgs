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
	libgs::utils::process aaaa;

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		libgs::error_code error;
		auto aaa = co_await aaaa.join(libgs::use_awaitable | error);

		char buffer[1024] {0};
		auto bbb = co_await aaaa.read({buffer, 1024}, libgs::use_awaitable | error);

		auto ccc = co_await aaaa.write({buffer, 111}, libgs::use_awaitable | error);

		// auto ccc = co_await aaaa.run({buffer, 111}, libgs::use_awaitable | error);

		auto ddd = aaaa.run();
		auto eee = aaaa.run(libgs::use_awaitable | error);

		co_return ;
	});

	// libgs::utils::process::exec (
	// 	"lshw 2>/dev/null "
	// 	" | grep -m 1 'serial:' "
	// 	" | awk '{print substr($2, length($2)-7)}' "
	// )
	// .transform([](int code)
	// {
	// 	spdlog::info("exit code: {}", code);
	// })
	// .or_else([](const libgs::error_code &error)
	// {
	// 	spdlog::error("process crashed: {}", error);
	// });
	return 0;
}