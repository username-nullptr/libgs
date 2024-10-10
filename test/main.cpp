// #include <libgs.h>
#include <libgs/core.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);


	return libgs::execution::exec();
//	return 0;
}
