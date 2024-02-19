#include <iostream>
#include <libgs/core/log.h>
#include <libgs/io/tcp_socket.h>
#include <libgs/io/timer.h>

using namespace std::chrono;

#if 0

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_socket socket;

		std::error_code error;
		// co_await socket.connect({"baidu.com", 80}, {2s, libgs::use_awaitable_e[error]});
		co_await socket.connect({"172.24.179.59", 6688}, {2s, libgs::use_awaitable_e[error]});

		libgs_log_debug("connect error: {}\n", error);

		for(int i=0; i<5; i++)
		{
			auto res = co_await socket.write("hello world", libgs::use_awaitable_e[error]);

			libgs_log_debug("write error: {}: {}", res, error);

			std::string echo;
			res = co_await socket.read(echo, libgs::use_awaitable_e[error]);

			libgs_log_debug("read error: {}: {}", res, error);
			libgs_log_debug("read: {}\n", echo);

			co_await libgs::co_sleep_for(500ms);
		}
		libgs_exe.exit(error.value());
	});
	return libgs_exe.exec();
}

#else

void do_write(libgs::io::tcp_socket &socket, int count = 0)
{
	if( count > 4 )
		return libgs_exe.exit(0);

	socket.write("hello world", [&socket, count](size_t res, const libgs::error_code &error)
	{
		libgs_log_debug("write error: {}: {}", res, error);

		auto echo = std::make_shared<char[]>(128);
		socket.read({echo.get(), 128}, [echo, &socket, count](size_t res, const libgs::error_code &error)
		{
			libgs_log_debug("read error: {}: {}", res, error);
			libgs_log_debug("read: {}\n", echo.get());

			auto timer = libgs::io::make_timer(500ms);
			timer->wait([timer, &socket, count](const libgs::error_code&) mutable {
				do_write(socket, ++count);
			});
		});
	});
};

int main()
{
	libgs::io::tcp_socket socket;

	socket.connect({"172.24.179.59", 6688}, {2s, [&](const libgs::error_code &error)
	{
		libgs_log_debug("connect error: {}\n", error);
		do_write(socket);
	}});

	return libgs_exe.exec();
}

#endif
