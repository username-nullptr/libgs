#include <libgs/utils/modules.h>
#include <spdlog/spdlog.h>

LIBGS_UTILS_MODULE_INIT("hello.world", [](const libgs::string_vector &args)
{
	spdlog::info("Hello world !!! - args: {}", args);
});