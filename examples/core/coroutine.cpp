#include <libgs/core/log.h>
#include <libgs/core/coroutine.h>

using namespace std::chrono_literals;

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs_log_debug() << "Start <0>...";
		co_await libgs::co_sleep_for(1s);

		libgs_log_debug() << "1 second passed...";
		co_await libgs::co_sleep_for(2s);

		libgs_log_debug() << "Another 2 seconds passed...";
		libgs_log_debug() << "End <0>...";

		libgs_log_debug("await thread task: {} ...", libgs::this_thread_id());
		co_await libgs::co_thread([]
		{
			libgs_log_debug("Thread <1>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(2s);

			libgs_log_debug() << "2 second passed...";
			libgs_log_debug() << "End <1>...";
		});

		libgs_log_debug("await std::future: {} ...", libgs::this_thread_id());
		auto future = std::async(std::launch::async,[]
		{
			libgs_log_debug("Thread <2>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(1.5s);

			libgs_log_debug() << "1.5 second passed...";
			libgs_log_debug() << "End <2>...";
		});
		co_await libgs::co_wait(future);

		libgs_log_debug() << "example finished...";
		libgs::execution::exit();
	});
	return libgs::execution::exec();
}