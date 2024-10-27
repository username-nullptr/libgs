#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_MODULE_INIT(0,[]
{
	spdlog::info("module 0 future 0 start initialization ...");

	return std::async(std::launch::async,[]
	{
		libgs::sleep_for(1s);
		spdlog::info("module 0 future 0 : sleep 1s.");

		libgs::sleep_for(1s);
		spdlog::info("module 0 future 0 : sleep 2s.");

		libgs::sleep_for(1s);
		spdlog::info("module 0 future 0 initialized (3s).");
	});
});
