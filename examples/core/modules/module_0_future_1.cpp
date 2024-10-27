#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

LIBGS_MODULE_INIT(0,[]
{
	spdlog::info("module 0 future 1 start initialization ...");

	return std::async(std::launch::async,[]
	{
		libgs::sleep_for(711ms);
		spdlog::info("module 0 future 1 : sleep 711ms.");

		libgs::sleep_for(427ms);
		spdlog::info("module 0 future 1 : sleep {}ms.", 711 + 427);

		libgs::sleep_for(636ms);
		spdlog::info("module 0 future 1 : sleep {}ms.", 711 + 427 + 636);

		libgs::sleep_for(549ms);
		spdlog::info("module 0 future 1 initialized ({}ms).", 711 + 427 + 636 + 549);
	});
});
