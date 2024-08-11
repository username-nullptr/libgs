#include <libgs.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);


	return libgs::execution::exec();
//	return 0;
}
