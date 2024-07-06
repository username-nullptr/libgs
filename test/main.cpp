#include <libgs/io/udp_socket.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		try {
			libgs::io::udp_socket socket;
			libgs::io::host_endpoint ep("127.0.0.1", 22222);

			auto res = co_await socket.open().write(ep, "hello world", 5s);
			spdlog::info("write: {}", res);

			std::string buf;
			res = co_await socket.read(ep, buf, 5s);
			spdlog::info("read: ({}): {}", res, buf);

			libgs::execution::exit(0);
		}
		catch(std::exception &ex)
		{
			spdlog::error("======== : {}", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
#else
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		try {
			libgs::io::udp_socket socket;
			socket.open().bind({libgs::io::address_v4(),22222});

			spdlog::info("udp server started.");
			for(;;)
			{
				libgs::io::host_endpoint ep;
				std::string buf;

				auto res = co_await socket.read(ep, buf);
				spdlog::info("read: {} : ({}): {}", ep, res, buf);

				res = co_await socket.write(ep, "sr:" + buf, 5s);
				spdlog::info("write: {}", res);
			}
		}
		catch(std::exception &ex)
		{
			spdlog::error("======== : {}", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
#endif
	return libgs::execution::exec();
}
