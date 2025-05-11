#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

LIBGS_MODULE_INIT(3, [](const libgs::string_vector &args)
{
	spdlog::info("Hello world !!! - args: {}", args);
});