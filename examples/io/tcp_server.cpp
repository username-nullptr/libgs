#include <libgs/core/log.h>
#include <libgs/io/tcp_server.h>

#if 0
void do_read(libgs::io::tcp_socket_ptr socket, libgs::io::ip_endpoint ep)
{
	auto buf = std::make_shared<char[4096]>();
	socket->async_read({buf.get(),4096}, [
		socket = std::move(socket), ep = std::move(ep), buf = std::move(buf)
	](std::size_t size, const std::error_code &error)
	{
		if( size == 0 )
			libgs_log_error("disconnected: {}", ep);
		else
		{
			libgs_log_debug("read ({}): {}", size, std::string_view(buf.get(), size));
			do_read(std::move(socket), std::move(ep));
		}
	});
}

void do_accept(libgs::io::tcp_server &server)
{
	server.async_accept([&server](libgs::io::tcp_socket_ptr socket, const std::error_code &error)
	{
		if( error )
		{
			libgs_log_error("server error: {}", error);
			return libgs::execution::exit(-1);
		}
		auto ep = socket->remote_endpoint();
		libgs_log_debug("new connction: {}", ep);

		do_read(std::move(socket), std::move(ep));
		do_accept(server);
	});
}
#endif

int main()
{
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_server server;
		try {
			server.bind({libgs::io::address_v4(),12345});
			for(;;)
			{
				auto socket = co_await server.co_accept();
				auto ep = socket->remote_endpoint();
				libgs_log_debug("new connction: {}", ep);

				libgs::co_spawn_detached([socket = std::move(socket), ep = std::move(ep)]()->libgs::awaitable<void>
				{
					try {
						char buf[4096] = "";
						for(;;)
						{
							auto res = co_await socket->co_read({buf,4096});
							if( res == 0 )
								break;
							libgs_log_debug("read ({}): {}", res, std::string_view(buf, res));
						}
					}
					catch( std::exception &ex ) {
						libgs_log_error("socket error: {}", ex);
					}
					libgs_log_error("disconnected: {}", ep);
					co_return ;
				},
				server.pool());
			}
		}
		catch(std::exception &ex)
		{
			libgs_log_error("server error: {}", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
#else
	libgs::io::tcp_server server;
	std::error_code error;

	server.bind({libgs::io::address_v4(),12345}, error);
	if( error )
	{
		libgs_log_error("server error: {}", error);
		return -1;
	}
	do_accept(server);
#endif
	return libgs::execution::exec();
}