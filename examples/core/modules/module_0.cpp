#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

LIBGS_MODULE_INIT(0,[]
{
	spdlog::info("module 0 initialized.");
});
