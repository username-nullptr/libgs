#include <libgs/io/udp_socket.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
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

			co_await libgs::co_sleep_for(10s);
			libgs::execution::exit(0);
		}
		catch(std::exception &ex)
		{
			spdlog::error("======== : {}", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
	return libgs::execution::exec();
}