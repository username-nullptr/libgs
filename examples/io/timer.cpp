#include <libgs/io/timer.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::timer timer;
		spdlog::debug("timer start.");

		// std::error_code error;
		// co_await timer.co_wait({2s, error});
		try {
			co_await timer.wait(3s);
			spdlog::debug("timer finished.");
			libgs::execution::exit();
		}
		catch(std::exception &ex)
		{
			spdlog::debug("timer error: {}.", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
#else
	libgs::io::timer timer;
	spdlog::debug("timer start.");

	timer.wait({2s, [](const std::error_code &error)
	{
		spdlog::debug("timer finished. ({})", error);
		libgs::execution::exit(-error.value());
	}});
#endif
	return libgs::execution::exec();
}