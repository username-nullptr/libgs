#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

LIBGS_MODULE_INIT(1,[]
{
	spdlog::info("module 1 initialized.");
});
