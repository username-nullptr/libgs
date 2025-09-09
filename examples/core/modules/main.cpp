#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>
#include <iostream>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	std::cout << libgs::modules::sprint() << std::endl;

	libgs::modules::do_init();
	spdlog::info("modules initialized.");
	return 0;
}