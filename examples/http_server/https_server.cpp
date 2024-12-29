#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	constexpr unsigned short port = 12345;

	using namespace libgs::app::literals;
	asio::ssl::context ssl(asio::ssl::context::sslv23_server);
	try {
		ssl.set_default_verify_paths();
		ssl.set_verify_mode(asio::ssl::context::verify_client_once);
		ssl.set_options(asio::ssl::context::default_workarounds |
						asio::ssl::context::single_dh_use |
						asio::ssl::context::no_sslv2);

		ssl.set_password_callback([](std::size_t, asio::ssl::context::password_purpose){
			return "seri1234";
		});
		ssl.use_certificate_chain_file("~/.ssl/ssl.crt"_abs);
		ssl.use_private_key_file("~/.ssl/ssl.key"_abs, asio::ssl::context::pem);
	}
	catch(const std::exception &ex)
	{
		spdlog::error("ssl error: {}", ex);
		return -1;
	}
	libgs::https::server server({std::move(acceptor), ssl});
	server.bind({libgs::ip_type::v4, port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::https::server::context_t &context) -> libgs::awaitable<void>
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
			spdlog::debug("partial_body: {}\n", co_await request.read(asio::use_awaitable));

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world", asio::use_awaitable);
		co_return ;
	})
	.on_request<libgs::http::method::GET>("/aa*bb?cc/{arg0}/{arg1}",
	[](libgs::https::server::context_t &context) -> libgs::awaitable<void>
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
	[](libgs::https::server::context_t &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!", asio::use_awaitable);
		co_await context.response()
			.co_send_file("~/hello_world.txt");
//			.co_send_file("C:/Users/Administrator/Desktop/hello_world.txt");
		co_return ;
	})
	.on_exception([](libgs::https::server::context_t&, const std::exception &ex)
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

	spdlog::info("HTTPS Server started ({}) ...", port);
	return libgs::exec();
}
