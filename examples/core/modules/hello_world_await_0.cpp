#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

/* awiatable as the return value of the initializer
 * must be executed by co_run_init,
 * run_init execution will ignore it.
 * */
LIBGS_MODULE_INIT(3, []() -> libgs::awaitable<void>
{
	spdlog::info("hello world awaitable 0 start initialization ...");

	co_await libgs::sleep_for(1s, libgs::use_awaitable);
	spdlog::info("hello world awaitable 0 : sleep 1s.");

	co_await libgs::sleep_for(1s, libgs::use_awaitable);
	spdlog::info("hello world awaitable 0 : sleep 2s.");

	co_await libgs::sleep_for(1s, libgs::use_awaitable);
	spdlog::info("hello world awaitable 0 initialized (3s).");

	co_return ;
});
