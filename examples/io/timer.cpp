#include <libgs/core/log.h>
#include <libgs/io/timer.h>

using namespace std::chrono_literals;

int main()
{
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::timer timer;
		libgs_log_debug("timer start.");

		// std::error_code error;
		// co_await timer.co_wait({2s, error});
		try {
			co_await timer.wait(3s);
			libgs_log_debug("timer finished.");
			libgs::execution::exit();
		}
		catch(std::exception &ex)
		{
			libgs_log_debug("timer error: {}.", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
#else
	libgs::io::timer timer;
	libgs_log_debug("timer start.");

	timer.async_wait({2s, [](const std::error_code &error)
	{
		libgs_log_debug("timer finished. ({})", error);
		libgs::execution::exit(-error.value());
	}});
#endif
	return libgs::execution::exec();
}