#include <libgs/utils/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_UTILS_MODULE_INIT("module.3", []
{
	spdlog::info("module 1 future 0 start initialization ...");

	libgs::sleep_for(1s);
	spdlog::info("module 1 future 0 : sleep 1s.");

	libgs::sleep_for(2s);
	spdlog::info("module 1 future 0 : sleep 3s.");

	libgs::sleep_for(500ms);
	spdlog::info("module 1 future 0 initialized (3.5s).");

	spdlog::info("module 1 future 0 initialization finished.");
});
