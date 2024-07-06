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
			socket.open().bind({libgs::io::address_v4(),22222});

			spdlog::info("udp server started.");
			for(;;)
			{
				libgs::io::host_endpoint ep;
				std::string buf;

				auto res = co_await socket.read(ep, buf);
				spdlog::info("read: {} : ({}): {}", ep, res, buf);

				res = co_await socket.write(ep, "sr-: " + buf, 5s);
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
	return libgs::execution::exec();
}