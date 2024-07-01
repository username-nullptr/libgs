#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context &context) -> libgs::awaitable<void>
	{
		auto session = context.session();
		spdlog::debug("session: '{}': <{}>", session->id(), session);
		co_return ;
	})
	.on_exception([](libgs::http::server::context&, std::exception &ex)
	{
		spdlog::error(ex);
		return true;
	})
	.on_system_error([](std::error_code error)
	{
		spdlog::error(error);
		libgs::execution::exit(-1);
		return true;
	})
	.start();
	return libgs::execution::exec();
}