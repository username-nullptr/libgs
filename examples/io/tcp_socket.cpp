#include <libgs/core/log.h>
#include <libgs/io/tcp_socket.h>

using namespace std::chrono_literals;

int main()
{
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_socket socket;

		// std::error_code error;
		// co_await socket.co_connect({"127.0.0.1", 6688}, {10s, error});
		try {
			co_await socket.co_connect({"127.0.0.1", 6688}, 10s);
			co_await socket.co_write("hello world");

			char buf[128] = "";
			auto size = co_await socket.co_read({buf,128});

			libgs_log_debug("tcp_socket read: {}.", std::string_view(buf,size));
			libgs_exe.exit();
		}
		catch(std::exception &ex) 
		{
			libgs_log_error("tcp_socket error: {}.", ex);
			libgs_exe.exit(-1);
		}
		co_return ;
	});
#else
	libgs::io::tcp_socket socket;
	socket.async_connect({"127.0.0.1", 6688}, [&socket](const std::error_code &error)
	{
		if( error )
		{
			libgs_log_error("tcp_socket connect error: {}.", error);
			libgs_exe.exit(-error.value());
		}
		socket.async_write("hello world", [&socket](size_t, const std::error_code &error)
		{
			if( error )
			{
				libgs_log_error("tcp_socket write error: {}.", error);
				libgs_exe.exit(-error.value());
			}
			auto buf = std::make_shared<char[128]>();
			socket.async_read({buf.get(),128}, [buf = std::move(buf)](size_t size, const std::error_code &error)
			{
				if( error )
				{
					libgs_log_error("tcp_socket read error: {}.", error);
					libgs_exe.exit(-error.value());
				}
				libgs_log_debug("tcp_socket read: {}.", std::string_view(buf.get(),size));
				libgs_exe.exit();
			});
		});
	});
#endif
	return libgs_exe.exec();
}