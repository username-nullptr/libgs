// #include <libgs.h>
#include <libgs/core.h>
#include <spdlog/spdlog.h>

#include <libgs/http/client/session_pool.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	asio::io_context ioc;

	libgs::http::session_pool spool(ioc);


	return libgs::execution::exec();
//	return 0;
}
