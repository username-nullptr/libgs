#include <libgs/http/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::request &request, libgs::http::server::response &response) -> libgs::awaitable<void>
	{
		auto session_cookie =libgs::http::session::cookie_key();
		libgs_log_debug("cookie key: '{}'", session_cookie);

		auto session_id = request.cookie_or(session_cookie).to_string();
		libgs_log_debug("session id: '{}'", session_id);

		auto session = libgs::http::session::get_or_make(session_id);
		libgs_log_debug("session address: {}", session);

		response.set_cookie(session_cookie, {session->id()});
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