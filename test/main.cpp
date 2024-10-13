// #include <libgs.h>
#include <libgs/core.h>
#include <spdlog/spdlog.h>

// #include <libgs/http/client/request.h>
// #include <libgs/http/cxx/socket_session.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);


	return libgs::execution::exec();
//	return 0;
}
