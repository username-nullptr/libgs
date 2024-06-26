#include <libgs/core/log.h>
#include <libgs/io/udp_socket.h>

using namespace std::chrono_literals;

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		try {
			libgs::io::udp_socket socket;
			auto res = co_await socket.open().write({"127.0.0.1", 22222}, "hello world", 5s);

			libgs_log_info("res = {}", res);
			libgs::execution::exit(0);
		}
		catch(std::exception &ex)
		{
			libgs_log_error("======== : {}", ex);
			libgs::execution::exit(-1);
		}
	});
	return libgs::execution::exec();
}
