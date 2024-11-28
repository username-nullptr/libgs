#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_MODULE_INIT(3,[]
{
	spdlog::info("hello world future 0 start initialization ...");
	auto pool = std::make_shared<asio::thread_pool>(1);

	return libgs::co_spawn_future([pool]() -> libgs::awaitable<void>
	{
		co_await libgs::co_sleep_for(1s /*,*pool*/);
		spdlog::info("hello world future 0 : sleep 1s.");

		co_await libgs::co_sleep_for(1s /*,*pool*/);
		spdlog::info("hello world future 0 : sleep 2s.");

		co_await libgs::co_sleep_for(1s /*,*pool*/);
		spdlog::info("hello world future 0 initialized (3s).");

		co_return ;
	},
	*pool);
});
