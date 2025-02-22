#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	constexpr unsigned short port = 12345;
#if 1
	libgs::http::server server(std::move(acceptor));
	server.bind({libgs::ip_type::v4, port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
		auto &request = context.request();
		spdlog::debug("Version:{} - Method:{} - Path:{}",
					  request.version(),
					  method_string(request.method()),
					  request.path());

		for(auto &[key,value] : request.parameters())
			spdlog::debug("Parameter: {}: {}", key, value);

		for(auto &[key,value] : request.headers())
			spdlog::debug("Header: {}: {}", key, value);

		for(auto &[key,value] : request.cookies())
			spdlog::debug("Cookie: {}: {}", key, value);

		if( request.can_read_body() )
			spdlog::debug("partial_body: {}\n", co_await request.read(asio::use_awaitable));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world", asio::use_awaitable);
		co_return ;
	})
	.on_request<libgs::http::method::GET>("/aa*bb?cc/{arg0}/{arg1}",
	[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
		auto &request = context.request();
		for(auto &[key,value] : request.path_args())
			spdlog::debug("Path arg: {}: {}", key, value);

		spdlog::debug("Path arg0: {}", request.path_arg("arg0"));
		spdlog::debug("Path arg[0]: {}", request.path_arg(0));

		spdlog::debug("Path arg1: {}", request.path_arg("arg1"));
		spdlog::debug("Path arg[1]: {}", request.path_arg(1));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world", asio::use_awaitable);
		co_return ;
	})
	.on_request<libgs::http::method::GET>("/hello",
	[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!", asio::use_awaitable);
		co_await context.response()
			// .send_file("~/hello_world.txt", asio::use_awaitable);
			.send_file(L"C:/opt/data/秦岭.jpg", asio::use_awaitable);
		co_return ;
	})
	.on_service_error([](libgs::http::server::context_t&, const std::exception &ex)
	{
		spdlog::error("on_service_error: {}", ex);
//		return false; // Returning false will result in abort !!!
		return true;
	})
	.on_server_error([](std::error_code error)
	{
		spdlog::error("on_server_error: {}", error);
		libgs::exit(-1);
		return true;
	})
	.start();
#else
	libgs::http::wserver server(std::move(acceptor));
	server.bind({libgs::ip_type::v4, port})

	.on_request<libgs::http::method::GET>(L"/*",
	[](libgs::http::wserver::context_t &context) -> libgs::awaitable<void>
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
			spdlog::debug("partial_body: {}\n", co_await request.read(asio::use_awaitable));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response()._write("hello world");
		co_return ;
	})
	.on_request<libgs::http::method::GET>(L"/hello",
	[](libgs::http::wserver::context_t &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!", asio::use_awaitable);
		co_await context.response()
			.co_send_file("~/hello_world.txt");
//			.co_send_file("C:/Users/Administrator/Desktop/hello_world.txt");
		co_return ;
	})
	.on_exception([](libgs::http::wserver::context_t&, const std::exception &ex)
	{
		spdlog::error("on_exception: {}", ex);
//		return false; // Returning false will result in abort !!!
		return true;
	})
	.on_system_error([](std::error_code error)
	{
		spdlog::error("on_system_error: {}", error);
		libgs::exit(-1);
		return true;
	})
	.start();
#endif
	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::exec();
}