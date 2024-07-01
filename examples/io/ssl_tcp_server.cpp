#include <libgs/io/ssl_tcp_server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
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

		libgs::io::ssl_tcp_server server(ssl);
		try {
			server.bind({libgs::io::address_v4(),12345}).start();
			spdlog::info("Server started.");

			for(;;)
			{
				std::error_code error;
				auto socket = co_await server.accept(error);
				if( error )
				{
					if( error.value() == 336151574 ) // The client does not recognize the certificate.
					{
						spdlog::error(error);
						continue;
					}
					else
						throw std::system_error(error, "ssl_tcp_server::accept");
				}
				auto ep = socket.remote_endpoint();
				spdlog::debug("new connction: {}", ep);

				libgs::co_spawn_detached([socket = std::move(socket), ep = std::move(ep)]() mutable ->libgs::awaitable<void>
				{
					try {
						char buf[4096] = "";
						for(;;)
						{
							auto res = co_await socket.read_some({buf,4096});
							if( res == 0 )
								break;

							spdlog::debug("read ({}): {}", res, std::string_view(buf, res));
							co_await socket.write("HTTP/1.1 200 OK\r\n"
							                      "Content-Length: 3\r\n"
							                      "\r\n"
							                      "SSL");
						}
					}
					catch( std::exception &ex ) {
						spdlog::error("socket error: {}", ex);
					}
					spdlog::error("disconnected: {}", ep);
					co_return ;
				},
				server.pool());
			}
		}
		catch(std::exception &ex)
		{
			spdlog::error("server error: {}", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
	return libgs::execution::exec();
}
