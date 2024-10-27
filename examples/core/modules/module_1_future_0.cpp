#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_MODULE_INIT(1,[]
{
	spdlog::info("module 1 future 0 start initialization ...");

	return std::async(std::launch::async,[]
	{
		libgs::sleep_for(1s);
		spdlog::info("module 1 future 0 : sleep 1s.");

		libgs::sleep_for(2s);
		spdlog::info("module 1 future 0 : sleep 3s.");

		libgs::sleep_for(500ms);
		spdlog::info("module 1 future 0 initialized (3.5s).");
	});
});
