#include <libgs/utils/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_UTILS_MODULE_INIT("module.1", {.before = {"module.3"}}, []
{
	spdlog::info("module 0 future 0 start initialization ...");

	libgs::sleep_for(1s);
	spdlog::info("module 0 future 0 : sleep 1s.");

	libgs::sleep_for(1s);
	spdlog::info("module 0 future 0 : sleep 2s.");

	libgs::sleep_for(1s);
	spdlog::info("module 0 future 0 initialized (3s).");

	spdlog::info("module 0 future 0 initialization finished.");
});
