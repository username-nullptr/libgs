#include <libgs/utils/modules.h>
#include <spdlog/spdlog.h>
#include <iostream>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	std::cout << libgs::utils::modules::sprint() << std::endl;

	libgs::utils::modules::do_init();
	spdlog::info("modules initialized.");
	return 0;
}