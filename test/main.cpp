// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client.h>
// #include <libgs/http/client/request.h>

#include <list>
#include <iostream>
#include <libgs/http/server.h>

#include <libgs/core/observer.h>
#include <libgs/core/execution.h>
#include <libgs/coro/utils.h>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);


	return libgs::exec();
	// return 0;
}
