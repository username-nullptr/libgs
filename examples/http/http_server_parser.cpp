#include <libgs/http/server/request_parser.h>
#include <libgs/io/tcp_server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

libgs::awaitable<void> service(libgs::io::tcp_server::socket_t socket, libgs::io::ip_endpoint ep)
{
	libgs::http::request_parser parser;
	try {
		char rbuf[4096] = "";
		for(;;)
		{
			auto res = co_await socket.read_some({rbuf,4096}, 5s);
			if( res == 0 )
				break;

			if( not parser.append({rbuf,res}) or parser.can_read_body() )
				continue;

			spdlog::debug("Version:{} - Method:{} - Path:{}",
							parser.version(),
							libgs::http::to_method_string(parser.method()),
							parser.path());

			for(auto &[key,value] : parser.parameters())
				spdlog::debug("Parameter: {}: {}", key, value);

			for(auto &[key,value] : parser.headers())
				spdlog::debug("Header: {}: {}", key, value);

			for(auto &[key,value] : parser.cookies())
				spdlog::debug("Cookie: {}: {}", key, value);

			spdlog::debug("partial_body: {}\n", parser.take_partial_body());

			auto wbuf = std::format("HTTP/1.1 200 OK\r\n"
									"{}:close\r\n"
									"{}:11\r\n"
									"\r\n"
									"Hello world",
									libgs::http::header::connection,
									libgs::http::header::content_length);
			co_await socket.write(wbuf);
			co_await socket.close();
			break;
		}
	}
	catch( std::exception &ex ) {
		spdlog::error("socket error: {}", ex);
	}
	spdlog::error("disconnected: {}", ep);
	co_return ;
}

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_server server;
		try {
			server.bind({libgs::io::address_v4(),22222});
			for(;;)
			{
				auto socket = co_await server.accept();
				auto ep = socket.remote_endpoint();

				spdlog::debug("new connction: {}", ep);
				libgs::co_spawn_detached(service(std::move(socket), std::move(ep)), server.pool());
			}
		}
		catch(std::exception &ex)
		{
			spdlog::error("server error: {}", ex);
			libgs::execution::exit(-1);
		}
	});
	return libgs::execution::exec();
}