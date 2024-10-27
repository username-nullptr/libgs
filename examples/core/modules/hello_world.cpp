#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

LIBGS_MODULE_INIT(3,[]
{
	spdlog::info("Hello world !!!");
});