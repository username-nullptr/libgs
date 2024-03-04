#include <libgs/core/log.h>
#include <libgs/io/timer.h>

using namespace std::chrono;

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
			co_await timer.co_wait(2s);
			libgs_log_debug("timer finished.");
		}
		catch(std::exception &ex) {
			libgs_log_debug("timer error: {}.", ex);
		}
		libgs_exe.exit();
		co_return ;
	});
#else
	libgs::io::timer timer;
	libgs_log_debug("timer start.");

	timer.async_wait({2s, [](const std::error_code &error)
	{
		libgs_log_debug("timer finished. ({})", error);
		libgs_exe.exit();
	}});
#endif
	return libgs_exe.exec();
}