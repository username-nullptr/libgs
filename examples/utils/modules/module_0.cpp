#include <libgs/utils/modules.h>
#include <spdlog/spdlog.h>

LIBGS_UTILS_MODULE_INIT("module.0", {.after = {"hello.world"}}, []
{
	spdlog::info("module 0 initialized.");
});
