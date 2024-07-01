#include <libgs/io/tcp_server.h>
#include <spdlog/spdlog.h>

#if 0
void do_read(libgs::io::tcp_server::socket_t socket, libgs::io::ip_endpoint ep)
{
	auto buf = std::make_shared<char[4096]>();
	socket.read_some({buf.get(),4096}, [
		socket = std::move(socket), ep = std::move(ep), buf = std::move(buf)
	](std::size_t size, const std::error_code &error) mutable
	{
		if( size == 0 )
			spdlog::error("disconnected: {}", ep);
		else
		{
			spdlog::debug("read ({}): {}", size, std::string_view(buf.get(), size));
			do_read(std::move(socket), std::move(ep));
		}
	});
}

void do_accept(libgs::io::tcp_server &server)
{
	server.accept([&server](libgs::io::tcp_server::socket_t socket, const std::error_code &error)
	{
		if( error )
		{
			spdlog::error("server error: {}", error);
			return libgs::execution::exit(-1);
		}
		auto ep = socket.remote_endpoint();
		spdlog::debug("new connction: {}", ep);

		do_read(std::move(socket), std::move(ep));
		do_accept(server);
	});
}
#endif

int main()
{
	spdlog::set_level(spdlog::level::trace);
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_server server;
		try {
			server.bind({libgs::io::address_v4(),12345}).start();
			spdlog::info("Server started.");

			for(;;)
			{
				auto socket = co_await server.accept();
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
												  "Content-Length: 2\r\n"
												  "\r\n"
												  "Yh");
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
#else
	libgs::io::tcp_server server;
	std::error_code error;

	server.bind({libgs::io::address_v4(),12345}, error).start();
	if( error )
	{
		spdlog::error("server error: {}", error);
		return -1;
	}
	do_accept(server);
	spdlog::info("Server started.");
#endif
	return libgs::execution::exec();
}