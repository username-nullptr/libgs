#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>
#include <libgs/http/server/session_set.h>
#include <libgs/http/server/context.h>
#include <libgs/http/server/aop.h>
#include <libgs/http/server/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

class my_session : public libgs::http::session
{
public:
	using libgs::http::session::basic_session;
};

int main()
{
	spdlog::set_level(spdlog::level::trace);

	asio::ip::tcp::acceptor acceptor(libgs::execution::get_executor());
	libgs::http::server server(std::move(acceptor));

	constexpr unsigned short port = 12345;
	server.bind({asio::ip::tcp::v4(), port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context &context) -> libgs::awaitable<void>
	{
		auto &request = context.request();
		spdlog::debug("Version:{} - Method:{} - Path:{}",
					  request.version(),
					  libgs::http::to_method_string(request.method()),
					  request.path());

		for(auto &[key,value] : request.parameters())
			spdlog::debug("Parameter: {}: {}", key, value);

		for(auto &[key,value] : request.headers())
			spdlog::debug("Header: {}: {}", key, value);

		for(auto &[key,value] : request.cookies())
			spdlog::debug("Cookie: {}: {}", key, value);

		if( request.can_read_body() )
			spdlog::debug("partial_body: {}\n", co_await request.async_read_all(asio::use_awaitable));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world");
		co_return ;
	})
	.on_request<libgs::http::method::GET>("/hello",
	[](libgs::http::server::context &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!");
		co_await context.response()
		.async_send_file("C:/Users/Administrator/Desktop/hello_world.txt", asio::use_awaitable);
		co_return ;
	})
	.on_exception([](libgs::http::server::context&, std::exception &ex)
	{
		spdlog::error("on_exception: {}", ex);
//		return false; // Returning false will result in abort !!!
		return true;
	})
	.on_system_error([](std::error_code error)
	{
		spdlog::error("on_system_error: {}", error);
		libgs::execution::exit(-1);
		return true;
	})
	.start();

	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::execution::exec();
}
