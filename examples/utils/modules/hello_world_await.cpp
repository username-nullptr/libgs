#include <spdlog/spdlog.h>
#include <libgs/utils/modules.h>
#include <libgs/coro.h>

using namespace std::chrono_literals;
using namespace libgs::coro::literals;

LIBGS_UTILS_MODULE_INIT("hello.world.await", []
{
	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		spdlog::info("hello world awaitable 0 start initialization ...");

		co_await 1_s;
		spdlog::info("hello world awaitable 0 : sleep 1s.");

		co_await 1_s;
		spdlog::info("hello world awaitable 0 : sleep 2s.");

		co_await 1_s;
		spdlog::info("hello world awaitable 0 initialized (3s).");

		co_return ;
	},
	libgs::use_sync);
});
