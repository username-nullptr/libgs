#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::execution::get_executor());
	constexpr unsigned short port = 12345;
#if 1
	libgs::http::server server(std::move(acceptor));
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
			.async_send_file("~/hello_world.txt", asio::use_awaitable);
//			.async_send_file("C:/Users/Administrator/Desktop/hello_world.txt", asio::use_awaitable);
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
#else
	libgs::http::wserver server(std::move(acceptor));
	server.bind({asio::ip::tcp::v4(), port})

	.on_request<libgs::http::method::GET>(L"/*",
	[](libgs::http::wserver::context &context) -> libgs::awaitable<void>
	{
		auto &request = context.request();
		spdlog::debug("Version:{} - Method:{} - Path:{}",
					  libgs::wcstombs(request.version()),
					  libgs::http::to_method_string(request.method()),
					  libgs::wcstombs(request.path()));

		for(auto &[key,value] : request.parameters())
			spdlog::debug("Parameter: {}: {}", libgs::wcstombs(key), libgs::wcstombs(value.to_string()));

		for(auto &[key,value] : request.headers())
			spdlog::debug("Header: {}: {}", libgs::wcstombs(key), libgs::wcstombs(value.to_string()));

		for(auto &[key,value] : request.cookies())
			spdlog::debug("Cookie: {}: {}", libgs::wcstombs(key), libgs::wcstombs(value.to_string()));

		if( request.can_read_body() )
			spdlog::debug("partial_body: {}\n", co_await request.async_read_all(asio::use_awaitable));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world");
		co_return ;
	})
	.on_request<libgs::http::method::GET>(L"/hello",
	[](libgs::http::wserver::context &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!");
		co_await context.response()
			.async_send_file("~/hello_world.txt", asio::use_awaitable);
//			.async_send_file("C:/Users/Administrator/Desktop/hello_world.txt", asio::use_awaitable);
		co_return ;
	})
	.on_exception([](libgs::http::wserver::context&, std::exception &ex)
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
#endif
	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::execution::exec();
}