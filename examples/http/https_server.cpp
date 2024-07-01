#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::ssl::context ssl(libgs::ssl::context::sslv23_server);
	ssl.set_default_verify_paths();

	ssl.set_verify_mode(libgs::ssl::context::verify_client_once);
	ssl.set_options(libgs::ssl::context::default_workarounds |
	                libgs::ssl::context::single_dh_use |
	                libgs::ssl::context::no_sslv2);

	ssl.set_password_callback([](std::size_t, libgs::ssl::context::password_purpose){
		return "seri1234";
	});
	ssl.use_certificate_chain_file("/opt/openssl/install/bin/ssl.crt");
	ssl.use_private_key_file("/opt/openssl/install/bin/ssl.key", libgs::ssl::context::pem);

	libgs::https::server server(ssl);
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::https::server::context &context) -> libgs::awaitable<void>
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
			spdlog::debug("partial_body: {}\n", co_await request.read_all());

		// If you don't write anything, the server will write the default body for you
		// co_await context.response().write("hello world");
		co_return ;
	})
	.on_request<libgs::http::method::GET>("/hello",
	[](libgs::https::server::context &context) -> libgs::awaitable<void>
	{
//		co_await context.response().write("hello world !!!");
		co_await context.response().send_file("C:/Users/Administrator/Desktop/hello_world.txt");
	})
	.on_exception([](libgs::https::server::context&, std::exception &ex)
	{
		spdlog::error(ex);
//		return false; // Returning false will result in abort !!!
		return true;
	})
	.on_system_error([](std::error_code error)
	{
		spdlog::error(error);
		if( error.value() != 336151574 ) // The client does not recognize the certificate.
			libgs::execution::exit(-1);
		return true;
	})
	.start();
	spdlog::info("Server started.");
	return libgs::execution::exec();
}
