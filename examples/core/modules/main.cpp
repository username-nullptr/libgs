#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::modules::do_init();
	spdlog::info("modules initialized.");
	return 0;
}