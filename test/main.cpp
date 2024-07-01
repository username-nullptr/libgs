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
			auto res = co_await socket.open().write({"127.0.0.1", 22222}, "hello world", 5s);

			spdlog::info("res = {}", res);
			libgs::execution::exit(0);
		}
		catch(std::exception &ex)
		{
			spdlog::error("======== : {}", ex);
			libgs::execution::exit(-1);
		}
	});
	return libgs::execution::exec();
}
