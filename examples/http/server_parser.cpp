#include <libgs/http/server/parser.h>
#include <libgs/io/tcp_server.h>
#include <libgs/core/log.h>

libgs::awaitable<void> service(libgs::http::server_parser &parser, libgs::io::socket_ptr socket, libgs::io::ip_endpoint ep)
{
	try {
		char rbuf[4096] = "";
		for(;;)
		{
			auto res = co_await socket->co_read({rbuf,4096});
			if( res == 0 )
				break;

			if( not parser.append({rbuf,res}) )
				continue;
			auto datagram = parser.get_result();

			libgs_log_debug("Version:{} - Method:{} - Path:{}",
							datagram.version,
							libgs::http::to_method_string(datagram.method),
							datagram.path);

			for(auto &[key,value] : datagram.parameters)
				libgs_log_debug("Parameter: {}: {}", key, value);

			for(auto &[key,value] : datagram.headers)
				libgs_log_debug("Header: {}: {}", key, value);

			for(auto &[key,value] : datagram.cookies)
				libgs_log_debug("Cookie: {}: {}", key, value);

			libgs_log_debug("partial_body: {}", datagram.partial_body);

			auto wbuf = std::format("HTTP/1.1 200 OK\r\n"
									"{}:close\r\n"
									"{}:11\r\n"
									"\r\n"
									"Hello world",
									libgs::http::header::connection,
									libgs::http::header::content_length);
			co_await socket->co_write(wbuf);
			socket->close();
			break;
		}
	}
	catch( std::exception &ex ) {
		libgs_log_error("socket error: {}", ex);
	}
	libgs_log_error("disconnected: {}", ep);
	co_return ;
}

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::http::server_parser parser;
		libgs::io::tcp_server server;
		try {
			server.bind({libgs::io::address_v4(),22222});
			for(;;)
			{
				auto socket = co_await server.co_accept();
				auto ep = socket->remote_endpoint();

				libgs_log_debug("new connction: {}", ep);
				libgs::co_spawn_detached(service(parser, std::move(socket), std::move(ep)), server.pool());
			}
		}
		catch(std::exception &ex)
		{
			libgs_log_error("server error: {}", ex);
			libgs::execution::exit(-1);
		}
	});
	return libgs::execution::exec();
}