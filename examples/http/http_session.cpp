#include <libgs/http/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::service_context &context) -> libgs::awaitable<void>
	{
		auto session = context.session();
		libgs_log_debug("session: '{}': <{}>", session->id(), session);
		co_return ;
	})
	.on_error([](std::error_code error) -> libgs::awaitable<void>
	{
		libgs_log_error() << error;
		libgs::execution::exit(-1);
		co_return ;
	})
	.start();
	return libgs::execution::exec();
}