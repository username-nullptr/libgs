#include <libgs/http/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context &context) -> libgs::awaitable<void>
	{
		auto session = context.session();
		libgs_log_debug("session: '{}': <{}>", session->id(), session);
		co_return ;
	})
	.on_exception([](libgs::http::server::context&, std::exception &ex)
	{
		libgs_log_error() << ex;
		return true;
	})
	.on_system_error([](std::error_code error)
	{
		libgs_log_error() << error;
		libgs::execution::exit(-1);
		return true;
	})
	.start();
	return libgs::execution::exec();
}