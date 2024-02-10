#include <iostream>
#include <libgs/io/tcp_socket.h>
#include <libgs/core/log.h>

using namespace std::chrono;

int main()
{
	libgs::io::tcp_socket socket;

#if 0

	auto error = socket.connect("172.30.79.156", 6688);
	if( error )
	{
		libgs_log_debug("000000000 {}", error);
		return -1;
	}
	socket.write_some("hello world", &error);
	if( error )
	{
		libgs_log_debug("11111111111111 {}", error);
		return -2;
	}
	std::string buf;
	socket.read_some(buf, 1024, &error);
	if( error )
	{
		libgs_log_debug("222222222 {}", error);
		return -3;
	}
	libgs_log_debug("back: {}", buf);
	return 0;

#elif 0

	socket.async_connect("172.30.79.156", 6688, [&](const std::error_code &error)
	{
		if( error )
		{
			libgs_log_debug("000000000 {}", error);
			return libgs_exe.exit(-1);
		}
		socket.async_write_some("hello world", [&socket](std::size_t, const std::error_code &error)
		{
			if( error )
			{
				libgs_log_debug("11111111111111 {}", error);
				return libgs_exe.exit(-2);
			}
			auto buf = std::make_shared<char[1024]>();
			socket.async_read_some(buf.get(), 1024, [buf, &socket](std::size_t size, const std::error_code &error)
			{
				if( error )
				{
					libgs_log_debug("222222222 {}", error);
					return libgs_exe.exit(-3);
				}
				libgs_log_debug("back: {}", std::string(buf.get(), size));
				return libgs_exe.exit();
			});
		});
	});
	return libgs_exe.exec();

#else

	libgs::co_spawn_detached([&]() -> libgs::awaitable<void>
	{
		auto error = co_await socket.co_connect("172.30.79.156", 6688);
		if( error )
		{
			libgs_log_debug("000000000 {}", error);
			co_return libgs_exe.exit(-1);
		}
		co_await socket.co_write_some("hello world", &error);
		if( error )
		{
			libgs_log_debug("11111111111111 {}", error);
			co_return libgs_exe.exit(-2);
		}
		std::string buf;
		co_await socket.co_read_some(buf, 1024, &error);
		if( error )
		{
			libgs_log_debug("222222222 {}", error);
			co_return libgs_exe.exit(-3);
		}
		libgs_log_debug("back: {}", buf);
		co_return libgs_exe.exit();
	});
	return libgs_exe.exec();

#endif
}
