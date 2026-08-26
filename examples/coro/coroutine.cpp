#include <libgs/coro.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;
using namespace libgs::coro::literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::thread_pool pool;

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		spdlog::debug("Start <0>...");
		co_await 1_s;

		spdlog::debug("1 second passed...");
		co_await 2_s;

		spdlog::debug("Another 2 seconds passed...");
		spdlog::debug("End <0>...");

		spdlog::debug("await thread task: {} ...", libgs::this_thread_id());
		co_await libgs::local_dispatch([]
		{
			spdlog::debug("Thread <1>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(2s);

			spdlog::debug("2 second passed...");
			spdlog::debug("End <1>...");
		},
		libgs::use_awaitable);

		spdlog::debug("await std::future: {} ...", libgs::this_thread_id());
		auto future = std::async(std::launch::async,[]
		{
			spdlog::debug("Thread <2>: {} ...", libgs::this_thread_id());
			libgs::sleep_for(1.5s);

			spdlog::debug("1.5 second passed...");
			spdlog::debug("End <2>...");
		});
		co_await libgs::coro::wait(future);

		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		auto preExec = co_await libgs::coro::goto_thread();
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		co_await libgs::coro::goto_exec(pool);
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		co_await libgs::coro::goto_exec(preExec);
		spdlog::debug("run in thread: {}", libgs::this_thread_id());

		spdlog::debug("example finished...");
		co_await 5_s;

		libgs::exit();
		co_return ;
	});
	return libgs::exec();
}