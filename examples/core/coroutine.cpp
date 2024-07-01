#include <libgs/core/coroutine.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::thread_pool pool;

	libgs::co_spawn_detached([&]() -> libgs::awaitable<void>
	{
		spdlog::debug("Start <0>...");
		co_await libgs::co_sleep_for(1s);

		spdlog::debug("1 second passed...");
		co_await libgs::co_sleep_for(2s);

		spdlog::debug("Another 2 seconds passed...");
		spdlog::debug("End <0>...");

		spdlog::debug("await thread task: {} ...", libgs::this_thread_id());
		co_await libgs::co_thread([]
		{
			spdlog::debug("Thread <1>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(2s);

			spdlog::debug("2 second passed...");
			spdlog::debug("End <1>...");
		});

		spdlog::debug("await std::future: {} ...", libgs::this_thread_id());
		auto future = std::async(std::launch::async,[]
		{
			spdlog::debug("Thread <2>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(1.5s);

			spdlog::debug("1.5 second passed...");
			spdlog::debug("End <2>...");
		});
		co_await libgs::co_wait(future);

		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		co_await libgs::co_to_thread();
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		co_await libgs::co_to_exec(pool);
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		co_await libgs::co_to_exec();
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		spdlog::debug("example finished...");
		co_await libgs::co_sleep_for(60s);
		libgs::execution::exit();
	});
	return libgs::execution::exec();
}